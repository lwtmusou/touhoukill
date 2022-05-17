#include "player.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "engine.h"
#include "exppattern.h"
#include "general.h"
#include "mode.h"
#include "serverinfostruct.h"
#include "skill.h"
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
    QMultiMap<QString, int> disableShow;

    // original mark is only visible if it starts with '@'. The refactor proposal is to make mark always visible
    QMap<QString, int> marks;
    QSet<QString> flags;
    QHash<QString, int> history;
    QMap<QString, IdSet> piles;
    QMap<QString, int> pilesLength;
    QSet<QString> acquiredSkills; // Acquired skills isn't split into parts by game rule
    QList<QMap<QString, bool>> generalCardSkills; // General card skill
    QStringList invalidSkills;
    IdSet shownHandcards;
    IdSet brokenEquips;

    IdSet handcards; // a.k.a. knownHandCards in client side

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
    int chaoren; // for skill "chaoren"

    QMap<QString, QStringList> pile_open; // Fs: only used to classify move in server side. Not that important since secret pile should remain its security.
    QStringList skills_originalOrder, skills2_originalOrder; //equals  skills.keys().  unlike QMap, QStringList will keep originalOrder // Fs: removed due to meanless
    bool owner; // Fs: for request & response
    int initialSeat; // for record
    QString next; // the seat information is only maintained by room now. Player only remain a property called 'seat' which is used for record / log.

    // ------ ServerPlayer ------
    // following 6 are for communication
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;
    ClientSocket *socket;
    QString m_clientResponseString;
    QVariant _m_clientResponse;
    bool ready;
    Room *room; // replaced by RoomObject / later GameState
    Recorder *recorder; // ??
    // following 3 are for turn processing, maybe added later
    QList<Phase> phases;
    int _m_phases_index;
    QList<PhaseStruct> _m_phases_state;
    QStringList selected; // 3v3 mode use only
    QDateTime test_time; // unknown

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
    if (hasValidSkill(QStringLiteral("banling"))) {
        if (d->renhp != hp) {
            d->renhp = hp;
        }
        if (d->linghp != hp) {
            d->linghp = hp;
        }
    }
}

int Player::hp() const
{
    if (hasValidSkill(QStringLiteral("huanmeng")))
        return 0;
    return d->hp;
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

int Player::renHp() const
{
    return d->renhp;
}

int Player::lingHp() const
{
    return d->linghp;
}

int Player::dyingFactor() const
{
    return d->dyingFactor;
}

const IdSet &Player::shownHandcards() const
{
    return d->shownHandcards;
}

void Player::setShownHandcards(const IdSet &ids)
{
    d->shownHandcards = ids;
}

bool Player::isShownHandcard(int id) const
{
    if (d->shownHandcards.isEmpty() || id < 0)
        return false;
    return d->shownHandcards.contains(id);
}

const IdSet &Player::brokenEquips() const
{
    return d->brokenEquips;
}

void Player::setBrokenEquips(const IdSet &ids)
{
    d->brokenEquips = ids;
}

bool Player::isBrokenEquip(int id, bool consider_shenbao) const
{
    if (d->brokenEquips.isEmpty() || id < 0)
        return false;

    if (consider_shenbao)
        return d->brokenEquips.contains(id) && !hasValidSkill(QStringLiteral("shenbao"), false, false);
    return d->brokenEquips.contains(id);
}

int Player::maxHp() const
{
    if (hasValidSkill(QStringLiteral("huanmeng")))
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

int Player::lostHp() const
{
    return d->maxHp - qMax(hp(), 0);
}

bool Player::isWounded() const
{
    if (d->hp < 0)
        return true;
    else
        return hp() < d->maxHp;
}

Gender Player::gender() const
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

int Player::seat() const
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
    return qAbs(d->seat - another->seat()) == 1 || (d->seat == 1 && another->seat() == alive_length) || (d->seat == alive_length && another->seat() == 1);
}

bool Player::isAlive() const
{
    return d->alive;
}

void Player::setAlive(bool alive)
{
    d->alive = alive;
}

QStringList Player::flagList() const
{
    return d->flags.values();
}

void Player::setFlag(const QString &flag)
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

int Player::attackRange(bool include_weapon) const
{
    if (hasFlag(QStringLiteral("InfinityAttackRange")) || mark(QStringLiteral("InfinityAttackRange")) > 0)
        return 1000;

    include_weapon = include_weapon && d->weapon != -1;

    int fixeddis = d->room->correctAttackRange(this, include_weapon, true);
    if (fixeddis > 0)
        return fixeddis;

    int original_range = 1;
    int weapon_range = 0;

    if (include_weapon) {
        const Weapon *face = dynamic_cast<const Weapon *>(d->room->card(d->weapon)->face());
        Q_ASSERT(face);
        if (!isBrokenEquip(d->weapon, true))
            weapon_range = face->range();
    }

    int real_range = qMax(original_range, weapon_range) + d->room->correctAttackRange(this, include_weapon, false);

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
    return distanceTo(other) <= attackRange();
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
    if (hasValidSkill(QStringLiteral("chuanwu")))
        distance_limit = qMax(other->hp(), 1);
    if (d->fixedDistance.contains(other)) {
        if (distance_limit > 0 && d->fixedDistance.value(other) > distance_limit)
            return distance_limit;
        else
            return d->fixedDistance.value(other);
    }

    int right = originalRightDistanceTo(other);
    int left = d->room->players(false, false).length() - right;
    int distance = qMin(left, right);

    distance += d->room->correctDistance(this, other);
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
        setKingdom(new_general->kingdom());
}

const General *Player::general(int pos) const
{
    if (d->generals.length() > pos)
        return d->generals.at(pos);
    return nullptr;
}

QList<const General *> Player::generals() const
{
    return d->generals;
}

QString Player::generalName(int pos) const
{
    const General *generalp = general(pos);
    return (generalp == nullptr) ? QString() : generalp->name();
}

QStringList Player::generalNames() const
{
    QStringList r;
    for (int i = 0; i < d->generals.length(); ++i)
        r << generalName(i);

    return r;
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

QString Player::roleString() const
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

Role Player::role() const
{
    return d->role;
}

bool Player::isCurrent() const
{
    return d->phase != PhaseNotActive;
}

bool Player::hasValidSkill(const QString &skill_name, bool include_lose, bool include_hidden) const
{
    return hasValidSkill(Sanguosha->skill(skill_name), include_lose, include_hidden);
}

// TODO: split logic of 'player have a certain skill' and 'a certian skill is valid'
bool Player::hasValidSkill(const Skill *skill, bool include_lose, bool include_hidden) const
{
    if (skill == nullptr)
        return false;

    QString skill_name = skill->name();

    //@todo: need check
    if (ServerInfo.GameMode->category() == ModeHegemony) {
        if (!include_lose && !hasEquipSkill(skill_name) && !acquiredSkills().contains(skill_name) && hasGeneralCardSkill(skill_name)
            && !canShowGeneral(QList<int> {findPositionOfGeneralOwningSkill(skill_name)}))
            return false;
        if (!include_lose && !hasEquipSkill(skill_name) && !skill->isEternal()) {
            if (isSkillInvalid(skill_name))
                return false;
        }

        // can't use "foreach (const QMap<QString, bool> &skillMap, d->skills)" here since the type contains comma
        foreach (const auto &skillMap, d->generalCardSkills) {
            if (skillMap.contains(skill_name))
                return skillMap.value(skill_name);
        }

        return d->acquiredSkills.contains(skill_name);
    }

    //Other modes
    //For skill "yibian" of reimu_god
    if (mark(QStringLiteral("@disableShowRole")) > 0 && !hasShownRole()) {
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

    foreach (const auto &skillMap, d->generalCardSkills) {
        if (skillMap.contains(skill_name))
            return true;
    }

    return d->acquiredSkills.contains(skill_name);
}

bool Player::hasValidLordSkill(const QString &skill_name, bool include_lose) const
{
    if (!hasValidSkill(skill_name, include_lose))
        return false;

    if (d->acquiredSkills.contains(skill_name))
        return true;

    QString mode = ServerInfo.GameModeStr;
    if (mode == QStringLiteral("06_3v3") || mode == QStringLiteral("06_XMode") || mode == QStringLiteral("02_1v1")
#if 0
            // todo: make this in serverinfo
        || Config.value(QStringLiteral("WithoutLordskill"), false).toBool()
#endif
    )
        return false;

    if (isLord()) {
        foreach (const auto skillMap, d->generalCardSkills) {
            if (skillMap.contains(skill_name))
                return true;
        }
    }

    return false;
}

bool Player::hasValidLordSkill(const Skill *skill, bool include_lose /* = false */) const
{
    if (skill == nullptr)
        return false;

    return hasValidLordSkill(skill->name(), include_lose);
}

void Player::setSkillInvalidity(const Skill *skill, bool invalidity)
{
    if (skill == nullptr)
        setSkillInvalidity(QStringLiteral("_ALL_SKILLS"), invalidity);
    else
        setSkillInvalidity(skill->name(), invalidity);
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

    return isSkillInvalid(skill->name());
}

bool Player::isSkillInvalid(const QString &skill_name) const
{
    if (skill_name != QStringLiteral("_ALL_SKILLS")) {
        const Skill *skill = Sanguosha->skill(skill_name);
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
    if (place >= d->generalCardSkills.length())
        return;

    const Skill *skill = Sanguosha->skill(skill_name);
    Q_ASSERT(skill);

    d->generalCardSkills[place][skill_name] = !skill->canPreshow() || d->generalShown[place];
}

void Player::loseSkill(const QString &skill_name, int place)
{
    if (place >= d->generalCardSkills.length())
        return;

    d->generalCardSkills[place].remove(skill_name);
}

void Player::setEquip(const Card *equip)
{
    const EquipCard *face = dynamic_cast<const EquipCard *>(equip->face());
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
    default:
        break;
    }
}

void Player::removeEquip(const Card *equip)
{
    const EquipCard *face = dynamic_cast<const EquipCard *>(equip->face());
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
    default:
        break;
    }
    if (d->brokenEquips.contains(equip->id()))
        d->brokenEquips.remove(equip->id());
}

bool Player::hasEquip(const Card *card) const
{
    Q_ASSERT(card != nullptr);
    QList<int> ids;
    if (card->isVirtualCard())
        ids << card->effectiveId();
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

const Card *Player::weapon() const
{
    if (d->weapon != -1)
        return d->room->card(d->weapon);
    return nullptr;
}

const Card *Player::armor() const
{
    if (d->armor != -1)
        return d->room->card(d->armor);
    return nullptr;
}

const Card *Player::defensiveHorse() const
{
    if (d->defensiveHorse != -1)
        return d->room->card(d->defensiveHorse);
    return nullptr;
}

const Card *Player::offensiveHorse() const
{
    if (d->offensiveHorse != -1)
        return d->room->card(d->offensiveHorse);
    return nullptr;
}

const Card *Player::treasure() const
{
    if (d->treasure != -1)
        return d->room->card(d->treasure);
    return nullptr;
}

QList<const Card *> Player::equipCards() const
{
    QList<const Card *> equips {weapon(), armor(), defensiveHorse(), offensiveHorse(), treasure()};
    equips.removeAll(nullptr);
    return equips;
}

IdSet Player::equips() const
{
    QList<int> equips {d->weapon, d->armor, d->defensiveHorse, d->offensiveHorse, d->treasure};
    equips.removeAll(-1);
    return List2Set(equips);
}

const Card *Player::equipCard(int index) const
{
    const Card *equip = nullptr;
    switch (index) {
    case 0:
        equip = weapon();
        break;
    case 1:
        equip = armor();
        break;
    case 2:
        equip = defensiveHorse();
        break;
    case 3:
        equip = offensiveHorse();
        break;
    case 4:
        equip = treasure();
        break;
    default:
        // TODO: Raise warning here.
        return nullptr;
    }
    return equip;
}

bool Player::hasValidWeapon(const QString &weapon_name) const
{
    if (mark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (d->room->treatAsEquipping(this, weapon_name, WeaponLocation) != nullptr)
        return true;

    if ((d->weapon == -1) || isBrokenEquip(d->weapon, true))
        return false;

    if (d->room->card(d->weapon)->faceName() == weapon_name || d->room->card(d->weapon)->face()->isKindOf(weapon_name))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->cardDescriptor(d->weapon);
    return real_weapon.face()->name() == weapon_name || real_weapon.face()->isKindOf(weapon_name);
}

bool Player::hasValidArmor(const QString &armor_name) const
{
    if (!tag[QStringLiteral("Qinggang")].toStringList().isEmpty() || mark(QStringLiteral("Armor_Nullified")) > 0 || mark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (d->room->treatAsEquipping(this, armor_name, ArmorLocation) != nullptr)
        return true;

    if ((d->armor == -1) || isBrokenEquip(d->armor, true))
        return false;

    if (d->room->card(d->armor)->faceName() == armor_name || d->room->card(d->armor)->face()->isKindOf(armor_name))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->cardDescriptor(d->armor);
    return real_weapon.face()->name() == armor_name || real_weapon.face()->isKindOf(armor_name);
}

bool Player::hasValidTreasure(const QString &treasure_name) const
{
    if (mark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (d->room->treatAsEquipping(this, treasure_name, TreasureLocation) != nullptr)
        return true;

    if ((d->treasure == -1) || isBrokenEquip(d->treasure, true))
        return false;

    if (d->room->card(d->treasure)->faceName() == treasure_name || d->room->card(d->treasure)->face()->isKindOf(treasure_name))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->cardDescriptor(d->treasure);
    return real_weapon.face()->name() == treasure_name || real_weapon.face()->isKindOf(treasure_name);
}

QList<const Card *> Player::judgingAreaCards() const
{
    QList<const Card *> cards;

    foreach (int card_id, d->judgingArea)
        cards.append(roomObject()->card(card_id));
    return cards;
}

QList<int> Player::judgingArea() const
{
    return d->judgingArea;
}

Phase Player::phase() const
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

int Player::maxCards(const QString &except) const
{
    int origin = d->room->correctMaxCards(this, true, except);
    if (origin == 0)
        origin = qMax(hp(), 0);
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
    extra += d->room->correctMaxCards(this, false, except);

    return qMax(origin + rule + extra, 0);
}

QString Player::kingdom() const
{
    if (d->kingdom.isEmpty() && (!d->generals.isEmpty()))
        return d->generals.first()->kingdom();
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
    return handcardNum() == 0;
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
        if (!to->shownHandcards().isEmpty())
            return true;
    } else if (flags.contains(QStringLiteral("h"))) {
        if ((to->handcardNum() - to->shownHandcards().size()) > 0)
            return true;
    }
    if (flags.contains(judging_flag) && !to->judgingAreaCards().isEmpty())
        return true;
    if (flags.contains(equip_flag)) {
        QSet<QString> Equips;
        if (to->weapon() != nullptr) {
            Equips << QStringLiteral("weapon");
        }
        if (to->armor() != nullptr) {
            Equips << QStringLiteral("armor");
        }
        if (to->defensiveHorse() != nullptr) {
            Equips << QStringLiteral("dh");
        }
        if (to->offensiveHorse() != nullptr) {
            Equips << QStringLiteral("oh");
        }
        if (to->treasure() != nullptr) {
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
        if ((to->weapon() != nullptr) && card_id == to->weapon()->effectiveId())
            return false;
    }

    if (this == to) {
        if (isCardLimited(roomObject()->card(card_id), MethodDiscard))
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
        const Card *trick = roomObject()->card(trick_id);
        // TODO: Wait! I don't know how to distinguish between card->name() and card->faceName()()
        // Fs: Just use a unified name! Don't you feel it's difficult to distinguish 2 names now?
        if (trick->faceName() == trick_name)
            return true;
    }
    return false;
}

int Player::handcardNum() const
{
    return d->handCardNum;
}

void Player::removeCard(const Card *card, Place place, const QString &pile_name)
{
    switch (place) {
    case PlaceHand: {
        if (d->handcards.contains(card->id()))
            d->handcards.remove(card->id());
        if (d->shownHandcards.contains(card->id()))
            d->shownHandcards.remove(card->id());
        d->handCardNum--;
        break;
    }
    case PlaceEquip: {
        const EquipCard *equip = dynamic_cast<const EquipCard *>(card->face());
        if (equip == nullptr)
            equip = dynamic_cast<const EquipCard *>(Sanguosha->cardDescriptor(card->effectiveId()).face());
        Q_ASSERT(equip != nullptr);
        equip->onUninstall(this);
        removeEquip(card);
        break;
    }
    case PlaceDelayedTrick: {
        removeDelayedTrick(card);
        break;
    }
    case PlaceSpecial: {
        int card_id = ((card == nullptr) ? -1 : card->effectiveId());
        if (card_id != -1) {
            QString n = pileName(card_id);
            if (!pile_name.isEmpty()) {
                if (pile_name != n) {
                    // ???
                }
            }
        }

        if (!pile_name.isEmpty()) {
            if (card_id != -1)
                d->piles[pile_name].remove(card_id);
            d->pilesLength[pile_name]--;
            if (d->pilesLength[pile_name] == 0) {
                d->piles.remove(pile_name);
                d->pilesLength.remove(pile_name);
            }
        }

        break;
    }
    default:
        break;
    }
}

void Player::addCard(const Card *card, Place place, const QString &pile_name)
{
    switch (place) {
    case PlaceHand: {
        if (card != nullptr)
            d->handcards << card->id();
        d->handCardNum++;
        break;
    }
    case PlaceEquip: {
        const EquipCard *equip = dynamic_cast<const EquipCard *>(card->face());
        if (equip == nullptr)
            equip = dynamic_cast<const EquipCard *>(Sanguosha->cardDescriptor(card->effectiveId()).face());
        setEquip(card);
        equip->onInstall(this);
        break;
    }
    case PlaceDelayedTrick: {
        addDelayedTrick(card);
        break;
    }
    case PlaceSpecial: {
        if (card != nullptr)
            d->piles[pile_name] << card->id();

        d->pilesLength[pile_name]++;
        break;
    }
    default:
        break;
    }
}

IdSet Player::handcards() const
{
    return d->handcards;
}

void Player::setHandCards(const IdSet &hc)
{
    // all cards are known before means all cards are known afterwards?
    // need to determine whether the design is appropriate
    bool allCardsAreKnown = (d->handcards.count() == d->handCardNum);

    d->handcards = hc;
    if (allCardsAreKnown)
        d->handCardNum = hc.count();
}

QList<const Card *> Player::handCards() const
{
    QList<const Card *> r;
    foreach (int id, d->handcards)
        r << d->room->card(id);

    return r;
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

int Player::mark(const QString &mark) const
{
    return d->marks.value(mark, 0);
}

QMap<QString, int> Player::marks() const
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
        bool res = distanceTo(other, rangefix) <= attackRange() + d->room->correctCardTarget(ModDistance, this, THIS_SLASH);
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
    int count = handcardNum();
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

IdSet Player::pile(const QString &pile_name) const
{
    if (pile_name == QStringLiteral("shown_card"))
        return shownHandcards();
    return d->piles[pile_name];
}

QStringList Player::pileNames() const
{
    QStringList names;
    foreach (QString pile_name, d->piles.keys())
        names.append(pile_name);
    return names;
}

QString Player::pileName(int card_id) const
{
    foreach (QString pile_name, d->piles.keys()) {
        const IdSet pile = d->piles[pile_name];
        if (pile.contains(card_id))
            return pile_name;
    }

    return QString();
}

IdSet Player::getHandPile() const
{
    IdSet result;
    foreach (const QString &p, pileNames()) {
        if (p.startsWith(QStringLiteral("&")) || (p == QStringLiteral("wooden_ox") && hasValidTreasure(QStringLiteral("wooden_ox")))) {
            foreach (int id, pile(p))
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
    foreach (const QString &pile, this->pileNames()) {
        if (pile.startsWith(QStringLiteral("&")) || pile.startsWith(QStringLiteral("^")))
            handlist.append(pile);
        else if (pile == QStringLiteral("wooden_ox") && hasValidTreasure(QStringLiteral("wooden_ox")))
            handlist.append(pile);
    }
    return handlist;
}

void Player::addHistory(const QString &name, int times)
{
    d->history[name] += times;
}

int Player::slashCount() const
{
    return d->history.value(QStringLiteral("Slash"), 0) + d->history.value(QStringLiteral("ThunderSlash"), 0) + d->history.value(QStringLiteral("FireSlash"), 0)
        + d->history.value(QStringLiteral("PowerSlash"), 0) + d->history.value(QStringLiteral("IronSlash"), 0) + d->history.value(QStringLiteral("LightSlash"), 0);
}

int Player::analapticCount() const
{
    return d->history.value(QStringLiteral("Analeptic"), 0) + d->history.value(QStringLiteral("MagicAnaleptic"), 0);
}

QHash<QString, int> Player::histories() const
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

    if (hasValidSkill(QStringLiteral("shenbao"))) {
        foreach (const Player *p, d->room->players(false, true)) {
            if (p == this)
                continue;
            if (p->hasEquipSkill(skill_name))
                return true;
        }
    }

    if (d->weapon != -1) {
        const Weapon *weaponc = dynamic_cast<const Weapon *>(d->room->card(d->weapon)->face());
        if ((Sanguosha->skill(weaponc) != nullptr) && Sanguosha->skill(weaponc)->name() == skill_name)
            return true;
    }
    if (d->armor != -1) {
        const Armor *armorc = dynamic_cast<const Armor *>(d->room->card(d->armor)->face());
        if ((Sanguosha->skill(armorc) != nullptr) && Sanguosha->skill(armorc)->name() == skill_name)
            return true;
    }
    if (d->treasure != -1) {
        const Treasure *treasurec = dynamic_cast<const Treasure *>(d->room->card(d->treasure)->face());
        if ((Sanguosha->skill(treasurec) != nullptr) && Sanguosha->skill(treasurec)->name() == skill_name)
            return true;
    }
    return false;
}

QSet<const Skill *> Player::skills(bool include_equip, bool include_acquired, const QList<int> &positions) const
{
    QSet<const Skill *> skillList;
    QSet<QString> skills;

    for (int i = 0; i < d->generalCardSkills.length(); ++i) {
        if (positions.isEmpty() || positions.contains(i))
            skills.unite(List2Set(d->generalCardSkills.value(i).keys()));
    }

    foreach (const auto &x, d->generalCardSkills)
        skills.unite(List2Set(x.keys()));

    if (include_acquired)
        skills.unite(d->acquiredSkills);

    foreach (const QString &skill_name, skills) {
        const Skill *skill = Sanguosha->skill(skill_name);
        if ((skill != nullptr) && (include_equip || !hasEquipSkill(skill->name())))
            skillList << skill;
    }

    return skillList;
}

const QSet<QString> &Player::acquiredSkills() const
{
    return d->acquiredSkills;
}

bool Player::isProhibited(const Player *to, const Card *card, const QList<const Player *> &others) const
{
    return d->room->isProhibited(this, to, card, others) != nullptr;
}

bool Player::canSlashWithoutCrossbow(const Card *slash) const
{
    const Card *newslash = roomObject()->cloneCard(QStringLiteral("Slash"));
#define THIS_SLASH (slash == NULL ? newslash : slash)
    int slash_count = slashCount();
    int valid_slash_count = 1;
    valid_slash_count += d->room->correctCardTarget(ModResidue, this, THIS_SLASH);
    roomObject()->cardDeleting(newslash);
    return slash_count < valid_slash_count;
#undef THIS_SLASH
}

bool Player::isLastHandCard(const Card *card, bool contain) const
{
    if (d->handCardNum == d->handcards.count()) {
        // all cards is known (on either client or server side)
        IdSet ids;
        if (!card->isVirtualCard())
            ids << card->id();
        else
            ids.unite(card->subcards());

        return contain ? ((ids + d->handcards) == ids) : (ids == d->handcards);
    }

    // else: assume not last hand card
    return false;
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
        HandlingMethod method = string2HandlingMethod(limit);
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
        HandlingMethod method = string2HandlingMethod(limit);
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
            const Card *c = roomObject()->card(card_id);
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
        HandlingMethod method = string2HandlingMethod(limit);
        if (d->cardLimitation[method].contains(reason))
            return true;
    }
    return false;
}

bool Player::haveShownSkill(const Skill *skill) const
{
    if (skill == nullptr)
        return false;

    if (d->acquiredSkills.contains(skill->name())) // deputy
        return true;

    if (skill->isEquipSkill())
        return true;

    if (skill->isAffiliatedSkill()) {
        const Skill *main_skill = skill->mainSkill();
        if (main_skill != nullptr)
            return haveShownSkill(main_skill);
        else
            return false;
    }

    for (int i = 0; i < d->generalCardSkills.length(); ++i) {
        if (d->generalShown.value(i, false) && d->generalCardSkills.value(i, {}).contains(skill->name()))
            return true;
    }

    return false;
}

bool Player::haveShownSkill(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->skill(skill_name);
    if (skill == nullptr)
        return false;

    return haveShownSkill(skill);
}

bool Player::haveShownSkills(const QString &skill_name) const
{
    foreach (const QString &skill, skill_name.split(QStringLiteral("|"))) {
        bool checkpoint = true;
        foreach (const QString &sk, skill.split(QStringLiteral("+"))) {
            if (!haveShownSkill(sk)) {
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
    for (auto it = d->generalCardSkills.begin(); it != d->generalCardSkills.end(); ++it) {
        if (it->contains(skill))
            (*it)[skill] = preshowed;
    }
}

void Player::setSkillsPreshowed(const QList<int> &positions, bool preshowed)
{
    foreach (int pos, positions) {
        auto &skills = d->generalCardSkills[pos];
        foreach (const QString &skill, skills.keys()) {
            if (!Sanguosha->skill(skill)->canPreshow())
                continue;
            skills[skill] = preshowed;
        }
    }
}

bool Player::havePreshownSkill(const QString &name) const
{
    foreach (const auto &x, d->generalCardSkills) {
        if (x.value(name, false))
            return true;
    }

    return false;
}

bool Player::havePreshownSkill(const Skill *skill) const
{
    return havePreshownSkill(skill->name());
}

bool Player::isHidden(int pos) const
{
    const auto &skills = d->generalCardSkills[pos];
    int count = 0;
    foreach (const QString &skillName, skills.keys()) {
        const Skill *skill = Sanguosha->skill(skillName);
        if (skill->canPreshow() && havePreshownSkill(skill->name()))
            return false;
        else if (!skill->canPreshow())
            ++count;
    }
    return count != skills.keys().length();
}

bool Player::haveShownGeneral(int pos) const
{
    return d->generalShown.value(pos);
}

void Player::setShownGeneral(int pos, bool show)
{
    d->generalShown[pos] = show;
}

bool Player::haveShownOneGeneral() const
{
    return d->generalShown.count(true) == 1;
}

bool Player::haveShownAllGenerals() const
{
    return d->generalShown.count(false) == 0;
}

bool Player::hasGeneralCardSkill(const QString &skill_name) const
{
    foreach (const auto &x, d->generalCardSkills) {
        if (x.contains(skill_name))
            return true;
    }

    return false;
}

bool Player::hasGeneralCardSkill(const Skill *skill) const
{
    return hasGeneralCardSkill(skill->name());
}

bool Player::isFriendWith(const Player *player, bool considerAnjiang) const
{
    Q_ASSERT(player);
    if (this == player)
        return true;

    if (player == nullptr)
        return false;

    if (ServerInfo.GameMode->category() != ModeHegemony)
        return false;

    if (considerAnjiang) {
        if (!player->haveShownOneGeneral() && this != player)
            return false;
    } else {
        if (!haveShownOneGeneral() || !player->haveShownOneGeneral())
            return false;
    }

    // TODO: refactor
    if (kingdom() == QStringLiteral("careerist") || player->kingdom() == QStringLiteral("careerist"))
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
    if (!player->haveShownOneGeneral())
        return false;

    if (!haveShownGeneral()) {
        QString role = roleString();
        int i = 1;
        foreach (const Player *p, d->room->players()) {
            if (p == this)
                continue;
            if (p->roleString() == role) {
                if (p->haveShownGeneral() && p->kingdom() != QStringLiteral("careerist"))
                    ++i;
            }
        }
        if (i > (parent()->findChildren<const Player *>().length() / 2))
            return false;
        else if (role == player->roleString())
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
    const Skill *skill = Sanguosha->skill(skill_name);
    if (skill == nullptr)
        return -1;

    if (skill->isAffiliatedSkill()) { //really confused about invisible skills! by weidouncle
        const Skill *main_skill = skill->mainSkill();
        if (main_skill != nullptr)
            return findPositionOfGeneralOwningSkill(main_skill->name());
    }

    for (int i = 0; i < d->generalCardSkills.length(); ++i) {
        if (d->generalCardSkills.value(i).contains(skill_name))
            return i;
    }

    return -1;
}

void Player::removeDisableShow(const QString &reason)
{
    while (d->disableShow.contains(reason))
        d->disableShow.remove(reason);
}

void Player::setDisableShow(const QList<int> &positions, const QString &reason)
{
    if (!positions.isEmpty()) {
        foreach (int position, positions)
            d->disableShow.insert(reason, position);
    } else {
        for (int i = 0; i < d->generals.length(); ++i)
            d->disableShow.insert(reason, i);
    }
}

QStringList Player::disableShow(int pos) const
{
    return d->disableShow.keys(pos);
}

bool Player::canShowGeneral(const QList<int> &positions) const
{
    for (int i = 0; i < d->generals.length(); ++i) {
        if (haveShownGeneral(i))
            continue;
        if (positions.isEmpty() || positions.contains(i)) {
            if (!disableShow(i).isEmpty())
                return true;
        }
    }

    return false;
}

QList<const Player *> Player::getFormation() const
{
    QList<const Player *> teammates;
    teammates << this;
    int n = d->room->players(false, false).length();
    int num = n;
    for (int i = 1; i < n; ++i) {
        const Player *target = getNextAlive(i);
        if (isFriendWith(target))
            teammates << target;
        else {
            num = i;
            break;
        }
    }

    n -= num;
    for (int i = 1; i < n; ++i) {
        const Player *target = getLastAlive(i);
        if (isFriendWith(target))
            teammates << target;
        else
            break;
    }

    return teammates;
}

void Player::addBrokenEquips(const IdSet &card_ids)
{
    foreach (int id, card_ids)
        d->brokenEquips << id;
}

void Player::removeBrokenEquips(const IdSet &card_ids)
{
    foreach (int id, card_ids)
        d->brokenEquips.remove(id);
}

void Player::addToShownHandCards(const IdSet &card_ids)
{
    IdSet add_ids;
    int newKnown = 0;
    foreach (int id, card_ids) {
        if (!d->shownHandcards.contains(id))
            add_ids << id;
        if (!d->handcards.contains(id))
            ++newKnown;
    }
    if (add_ids.isEmpty())
        return;

    if (newKnown + d->handcards.count() > d->handCardNum) {
        // warning?
    }

    d->shownHandcards.unite(add_ids);
    d->handcards.unite(add_ids);
}

void Player::removeShownHandCards(const IdSet &card_ids)
{
    foreach (int id, card_ids) {
        if (d->shownHandcards.contains(id))
            d->shownHandcards.remove(id);
    }
}
