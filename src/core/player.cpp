#include "player.h"
#include "client.h"
#include "engine.h"
#include "room.h"
#include "settings.h"

Player::Player(QObject *parent)
    : QObject(parent)
    , owner(false)
    , general(nullptr)
    , general2(nullptr)
    , m_gender(General::Sexless)
    , hp(-1)
    , max_hp(-1)
    , renhp(-1)
    , linghp(-1)
    , dyingFactor(0)
    , chaoren(-1)
    , role_shown(false)
    , state("online")
    , seat(0)
    , initialSeat(0)
    , alive(true)
    , general_showed(false)
    , general2_showed(false) //hegemony
    , phase(NotActive)
    , weapon(nullptr)
    , armor(nullptr)
    , defensive_horse(nullptr)
    , offensive_horse(nullptr)
    , treasure(nullptr)
    , face_up(true)
    , chained(false)
    , removed(false)
{
}

void Player::setScreenName(const QString &screen_name)
{
    this->screen_name = screen_name;
}

QString Player::screenName() const
{
    return screen_name;
}

bool Player::isOwner() const
{
    return owner;
}

void Player::setOwner(bool owner)
{
    if (this->owner != owner) {
        this->owner = owner;
        emit owner_changed(owner);
    }
}

bool Player::hasShownRole() const
{
    return role_shown;
}

void Player::setShownRole(bool shown)
{
    this->role_shown = shown;
}

void Player::setHp(int hp)
{
    bool changed = false;
    if (this->hp != hp) {
        this->hp = hp;
        changed = true;
    }
    if (hasSkill("banling")) {
        if (this->renhp != hp) {
            this->renhp = hp;
            changed = true;
        }
        if (this->linghp != hp) {
            this->linghp = hp;
            changed = true;
        }
    }
    if (changed)
        emit hp_changed();
}

int Player::getHp() const
{
    if (hasSkill("huanmeng"))
        return 0;
    return hp;
}

int Player::dyingThreshold() const
{
    int value = 1 + dyingFactor;
    foreach (const Player *p, getAliveSiblings()) {
        if (p->isCurrent() && p->hasSkill("yousi"))
            value = qMax(0, p->getHp());
    }
    return value;
}

void Player::setRenHp(int renhp)
{
    if (this->renhp != renhp) {
        this->renhp = renhp;
        if (qMin(this->linghp, this->renhp) != this->hp)
            this->hp = qMin(this->linghp, this->renhp);
        emit hp_changed();
    }
}

void Player::setLingHp(int linghp)
{
    if (this->linghp != linghp) {
        this->linghp = linghp;
        if (qMin(this->linghp, this->renhp) != this->hp)
            this->hp = qMin(this->linghp, this->renhp);
        emit hp_changed();
    }
}

void Player::setDyingFactor(int dyingFactor)
{
    if (this->dyingFactor != dyingFactor)
        this->dyingFactor = dyingFactor;
    emit hp_changed();
}

int Player::getRenHp() const
{
    return renhp;
}

int Player::getLingHp() const
{
    return linghp;
}

int Player::getDyingFactor() const
{
    return dyingFactor;
}

int Player::getChaoren() const
{
    return chaoren;
}

void Player::setChaoren(int chaoren)
{
    if (this->chaoren != chaoren) {
        this->chaoren = chaoren;
        emit chaoren_changed();
    }
}

const IDSet &Player::getShownHandcards() const
{
    return shown_handcards;
}

void Player::setShownHandcards(const IDSet &ids)
{
    this->shown_handcards = ids;
    emit showncards_changed();
}

bool Player::isShownHandcard(int id) const
{
    if (shown_handcards.isEmpty() || id < 0)
        return false;
    return shown_handcards.contains(id);
}

const IDSet &Player::getBrokenEquips() const
{
    return broken_equips;
}

void Player::setBrokenEquips(const IDSet &ids)
{
    this->broken_equips = ids;
    emit brokenEquips_changed();
}

bool Player::isBrokenEquip(int id, bool consider_shenbao) const
{
    if (broken_equips.isEmpty() || id < 0)
        return false;

    if (consider_shenbao)
        return broken_equips.contains(id) && !hasSkill("shenbao", false, false);
    return broken_equips.contains(id);
}

QStringList Player::getHiddenGenerals() const
{
    return hidden_generals;
}

void Player::setHiddenGenerals(const QStringList &generals)
{
    this->hidden_generals = generals;
}

bool Player::canShowHiddenSkill() const
{
    if (shown_hidden_general != nullptr) {
        const General *hidden = Sanguosha->getGeneral(shown_hidden_general);
        if (hidden)
            return false;
    }
    return !hidden_generals.isEmpty();
}

bool Player::isHiddenSkill(const QString &skill_name) const
{
    if (hasSkill(skill_name, false, false))
        return false;
    QString name = getShownHiddenGeneral();
    if (name != nullptr) {
        const General *hidden = Sanguosha->getGeneral(name);
        if (hidden && hidden->hasSkill(skill_name))
            return false;
    }
    return hasSkill(skill_name);
}

QString Player::getShownHiddenGeneral() const
{
    return shown_hidden_general;
}

void Player::setShownHiddenGeneral(const QString &general)
{
    this->shown_hidden_general = general;
}

int Player::getMaxHp() const
{
    if (hasSkill("huanmeng")) {
        return 0;
    }
    return max_hp;
}

void Player::setMaxHp(int max_hp)
{
    if (this->max_hp == max_hp)
        return;
    this->max_hp = max_hp;
    if (hp > max_hp)
        hp = max_hp;
    emit hp_changed();
}

int Player::getLostHp() const
{
    return max_hp - qMax(getHp(), 0);
}

bool Player::isWounded() const
{
    if (hp < 0)
        return true;
    else
        return getHp() < max_hp;
}

General::Gender Player::getGender() const
{
    return m_gender;
}

void Player::setGender(General::Gender gender)
{
    m_gender = gender;
}

bool Player::isMale() const
{
    return m_gender == General::Male;
}

bool Player::isFemale() const
{
    return m_gender == General::Female;
}

bool Player::isNeuter() const
{
    return m_gender == General::Neuter;
}

int Player::getSeat() const
{
    return seat;
}

void Player::setSeat(int seat)
{
    this->seat = seat;
}

int Player::getInitialSeat() const
{
    return initialSeat;
}

void Player::setInitialSeat(int seat)
{
    this->initialSeat = seat;
}

bool Player::isAdjacentTo(const Player *another) const
{
    int alive_length = 1 + getAliveSiblings().length();
    return qAbs(seat - another->seat) == 1 || (seat == 1 && another->seat == alive_length) || (seat == alive_length && another->seat == 1);
}

bool Player::isAlive() const
{
    return alive;
}

bool Player::isDead() const
{
    return !alive;
}

void Player::setAlive(bool alive)
{
    this->alive = alive;
}

QString Player::getFlags() const
{
    return QStringList(flags.values()).join("|");
}

QStringList Player::getFlagList() const
{
    return QStringList(flags.values());
}

void Player::setFlags(const QString &flag)
{
    if (flag == ".") {
        clearFlags();
        return;
    }
    static QChar unset_symbol('-');
    if (flag.startsWith(unset_symbol)) {
        QString copy = flag;
        copy.remove(unset_symbol);
        flags.remove(copy);
    } else {
        flags.insert(flag);
    }
}

bool Player::hasFlag(const QString &flag) const
{
    return flags.contains(flag);
}

void Player::clearFlags()
{
    flags.clear();
}

int Player::getAttackRange(bool include_weapon) const
{
    if (hasFlag("InfinityAttackRange") || getMark("InfinityAttackRange") > 0)
        return 1000;

    include_weapon = include_weapon && weapon != nullptr;

    int fixeddis = Sanguosha->correctAttackRange(this, include_weapon, true);
    if (fixeddis > 0)
        return fixeddis;

    int original_range = 1, weapon_range = 0;

    if (include_weapon) {
        const Weapon *face = dynamic_cast<const Weapon *>(weapon->face());
        Q_ASSERT(face);
        if (!isBrokenEquip(weapon->effectiveID(), true))
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
        fixed_distance.remove(player);
    else
        fixed_distance.insert(player, distance);
}

int Player::originalRightDistanceTo(const Player *other) const
{
    int right = 0;
    Player *next_p = parent()->findChild<Player *>(objectName());
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
    if (hasSkill("chuanwu"))
        distance_limit = qMax(other->getHp(), 1);
    if (fixed_distance.contains(other)) {
        if (distance_limit > 0 && fixed_distance.value(other) > distance_limit)
            return distance_limit;
        else
            return fixed_distance.value(other);
    }

    int right = originalRightDistanceTo(other);
    int left = aliveCount(false) - right;
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

void Player::setNext(Player *next)
{
    this->next = next->objectName();
}

void Player::setNext(const QString &next)
{
    this->next = next;
}

Player *Player::getNext(bool ignoreRemoved) const
{
    Player *next_p = parent()->findChild<Player *>(next);
    if (ignoreRemoved && next_p->isRemoved())
        return next_p->getNext(ignoreRemoved);
    return next_p;
}

QString Player::getNextName() const
{
    return next;
}

Player *Player::getLast(bool ignoreRemoved) const
{
    foreach (Player *p, parent()->findChildren<Player *>()) {
        if (p->getNext(ignoreRemoved) == this)
            return p;
    }
    return nullptr;
}

Player *Player::getNextAlive(int n, bool ignoreRemoved) const
{
    bool hasAlive = (aliveCount(!ignoreRemoved) > 0);
    Player *next = parent()->findChild<Player *>(objectName());
    if (!hasAlive)
        return next;
    for (int i = 0; i < n; ++i) {
        do
            next = next->getNext(ignoreRemoved);
        while (next->isDead());
    }
    return next;
}

Player *Player::getLastAlive(int n, bool ignoreRemoved) const
{
    return getNextAlive(aliveCount(!ignoreRemoved) - n, ignoreRemoved);
}

void Player::setGeneral(const General *new_general)
{
    if (general != new_general) {
        general = new_general;

        if (new_general && kingdom.isEmpty())
            setKingdom(new_general->getKingdom());

        emit general_changed();
    }
}

void Player::setGeneralName(const QString &general_name)
{
    const General *new_general = Sanguosha->getGeneral(general_name);
    Q_ASSERT(general_name.isNull() || general_name.isEmpty() || new_general != nullptr);
    setGeneral(new_general);
}

QString Player::getGeneralName() const
{
    if (general)
        return general->objectName();
    else
        return QString();
}

void Player::setGeneral2Name(const QString &general_name)
{
    const General *new_general = Sanguosha->getGeneral(general_name);
    if (general2 != new_general) {
        general2 = new_general;

        emit general2_changed();
    }
}

QString Player::getGeneral2Name() const
{
    if (general2)
        return general2->objectName();
    else
        return QString();
}

const General *Player::getGeneral2() const
{
    return general2;
}

QString Player::getFootnoteName() const
{
    if (general && general->objectName() != "anjiang")
        return getGeneralName();
    else if (general2 && general2->objectName() != "anjiang")
        return general2->objectName();
    else {
        return Sanguosha->translate(QString("SEAT(%1)").arg(QString::number(getInitialSeat())));
    }

    return getGeneralName();
}

QString Player::getState() const
{
    return state;
}

void Player::setState(const QString &state)
{
    if (this->state != state) {
        this->state = state;
        emit state_changed();
    }
}

void Player::setRole(const QString &role)
{
    if (this->role != role) {
        this->role = role;
        emit role_changed(role);
        if (isHegemonyGameMode(ServerInfo.GameMode)) {
            if (role == "careerist")
                emit kingdom_changed("careerist");
            else
                emit kingdom_changed(Sanguosha->GetMappedKingdom(role));
        }
    }
}

QString Player::getRole() const
{
    return role;
}

Player::Role Player::getRoleEnum() const
{
    static QMap<QString, Role> role_map;
    if (role_map.isEmpty()) {
        role_map.insert("lord", Lord);
        role_map.insert("loyalist", Loyalist);
        role_map.insert("rebel", Rebel);
        role_map.insert("renegade", Renegade);
    }

    return role_map.value(role);
}

const General *Player::getAvatarGeneral() const
{
    if (general)
        return general;

    QString general_name = property("avatar").toString();
    if (general_name.isEmpty())
        return nullptr;
    return Sanguosha->getGeneral(general_name);
}

const General *Player::getGeneral() const
{
    return general;
}

bool Player::isLord() const
{
    return getRole() == "lord";
}

bool Player::isCurrent() const
{
    return phase != Player::NotActive;
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
            && !canShowGeneral(inHeadSkills(skill_name) ? "h" : "d"))
            return false;
        if (!include_lose && !hasEquipSkill(skill_name) && !skill->isEternal()) {
            if (isSkillInvalid(skill_name))
                return false;
        }
        return skills.value(skill_name, false) || skills2.value(skill_name, false) || acquired_skills.contains(skill_name) || acquired_skills2.contains(skill_name);
    }

    //Other modes
    //For skill "yibian" of reimu_god
    if (getMark("@disableShowRole") > 0 && !hasShownRole()) {
        if (!skill->isEternal() && !skill->isAttachedSkill() && !hasEquipSkill(skill_name))
            return false;
    }

    //prevent infinite recursion
    if (include_hidden && !isSkillInvalid("anyun")
        && (skills.contains("anyun") || skills2.contains("anyun") || acquired_skills.contains("anyun") || acquired_skills2.contains("anyun")) && !skill->isLordSkill()
        && !skill->isAttachedSkill() && !skill->isLimited() && !skill->isEternal() && (skill->getShowType() != Skill::ShowStatic || hasFlag("has_anyu_state"))) {
        QString shown = shown_hidden_general;
        if (shown == nullptr) {
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
    foreach (QString skill, skill_name.split("|")) {
        bool checkpoint = true;
        foreach (QString sk, skill.split("+")) {
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
    if (general && general->hasSkill(skill_name))
        return true;

    if (general2 && general2->hasSkill(skill_name))
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

    QString mode = getGameMode();
    if (mode == "06_3v3" || mode == "06_XMode" || mode == "02_1v1" || Config.value("WithoutLordskill", false).toBool())
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
        setSkillInvalidity("_ALL_SKILLS", invalidity);

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
        return isSkillInvalid("_ALL_SKILLS");

    if (skill->isEternal() || skill->isAttachedSkill())
        return false;

    return isSkillInvalid(skill->objectName());
}

bool Player::isSkillInvalid(const QString &skill_name) const
{
    if (skill_name != "_ALL_SKILLS") {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill && (skill->isEternal() || skill->isAttachedSkill()))
            return false;
    }

    if (skill_invalid.contains("_ALL_SKILLS"))
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
    case RoundStart:
        return "round_start";
    case Start:
        return "start";
    case Judge:
        return "judge";
    case Draw:
        return "draw";
    case Play:
        return "play";
    case Discard:
        return "discard";
    case Finish:
        return "finish";
    case NotActive:
    default:
        return "not_active";
    }
}

void Player::setPhaseString(const QString &phase_str)
{
    static QMap<QString, Phase> phase_map;
    if (phase_map.isEmpty()) {
        phase_map.insert("round_start", RoundStart);
        phase_map.insert("start", Start);
        phase_map.insert("judge", Judge);
        phase_map.insert("draw", Draw);
        phase_map.insert("play", Play);
        phase_map.insert("discard", Discard);
        phase_map.insert("finish", Finish);
        phase_map.insert("not_active", NotActive);
    }

    setPhase(phase_map.value(phase_str, NotActive));
}

void Player::setEquip(const Card *equip)
{
    const EquipCard *face = dynamic_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);
    switch (face->location()) {
    case EquipCard::WeaponLocation:
        weapon = equip;
        break;
    case EquipCard::ArmorLocation:
        armor = equip;
        break;
    case EquipCard::DefensiveHorseLocation:
        defensive_horse = equip;
        break;
    case EquipCard::OffensiveHorseLocation:
        offensive_horse = equip;
        break;
    case EquipCard::TreasureLocation:
        treasure = equip;
        break;
    }
}

void Player::removeEquip(const Card *equip)
{
    const EquipCard *face = dynamic_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);
    switch (face->location()) {
    case EquipCard::WeaponLocation:
        weapon = nullptr;
        break;
    case EquipCard::ArmorLocation:
        armor = nullptr;
        break;
    case EquipCard::DefensiveHorseLocation:
        defensive_horse = nullptr;
        break;
    case EquipCard::OffensiveHorseLocation:
        offensive_horse = nullptr;
        break;
    case EquipCard::TreasureLocation:
        treasure = nullptr;
        break;
    }
}

bool Player::hasEquip(const Card *card) const
{
    Q_ASSERT(card != nullptr);
    int weapon_id = -1, armor_id = -1, def_id = -1, off_id = -1, tr_id = -1;
    if (weapon)
        weapon_id = weapon->effectiveID();
    if (armor)
        armor_id = armor->effectiveID();
    if (defensive_horse)
        def_id = defensive_horse->effectiveID();
    if (offensive_horse)
        off_id = offensive_horse->effectiveID();
    if (treasure)
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
    if (weapon)
        equips << weapon;
    if (armor)
        equips << armor;
    if (defensive_horse)
        equips << defensive_horse;
    if (offensive_horse)
        equips << offensive_horse;
    if (treasure)
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

bool Player::hasWeapon(const QString &weapon_name, bool, bool ignore_preshow) const
{
    if (getMark("Equips_Nullified_to_Yourself") > 0)
        return false;

    if (Sanguosha->ViewHas(this, weapon_name, "weapon", ignore_preshow) != nullptr)
        return true;

    if (!weapon || isBrokenEquip(weapon->effectiveID(), true))
        return false;
    if (weapon->faceName() == weapon_name || weapon->face()->isKindOf(weapon_name.toStdString().c_str()))
        return true;

    // TODO_Fs: Consider view-as weapon later
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(weapon->effectiveID());
    return real_weapon.face()->name() == weapon_name || real_weapon.face()->isKindOf(weapon_name.toStdString().c_str());
}

bool Player::hasArmorEffect(const QString &armor_name, bool) const
{
    if (!tag["Qinggang"].toStringList().isEmpty() || getMark("Armor_Nullified") > 0 || getMark("Equips_Nullified_to_Yourself") > 0)
        return false;

    if (Sanguosha->ViewHas(this, armor_name, "armor"))
        return true;

    if (!armor || isBrokenEquip(armor->effectiveID(), true))
        return false;
    if (armor->faceName() == armor_name || armor->face()->isKindOf(armor_name.toStdString().c_str()))
        return true;
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(weapon->effectiveID());
    return real_weapon.face()->name() == armor_name || real_weapon.face()->isKindOf(armor_name.toStdString().c_str());
}

bool Player::hasTreasure(const QString &treasure_name, bool) const
{
    if (getMark("Equips_Nullified_to_Yourself") > 0)
        return false;

    if (Sanguosha->ViewHas(this, treasure_name, "treasure"))
        return true;

    if (!treasure || isBrokenEquip(treasure->effectiveID(), true))
        return false;
    if (treasure->faceName() == treasure_name || treasure->face()->isKindOf(treasure_name.toStdString().c_str()))
        return true;
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(weapon->effectiveID());
    return real_weapon.face()->name() == treasure_name || real_weapon.face()->isKindOf(treasure_name.toStdString().c_str());
}

QList<const Card *> Player::getJudgingArea() const
{
    QList<const Card *> cards;
    foreach (int card_id, judging_area)
        cards.append(getRoomObject()->getCard(card_id));
    return cards;
}

QList<int> Player::getJudgingAreaID() const
{
    //for marshal
    return judging_area;
}

Player::Phase Player::getPhase() const
{
    return phase;
}

void Player::setPhase(Phase phase)
{
    this->phase = phase;
    emit phase_changed();
}

bool Player::isInMainPhase() const
{
    return phase == Start || phase == Judge || phase == Draw || phase == Play || phase == Discard || phase == Finish;
}

bool Player::faceUp() const
{
    return face_up;
}

void Player::setFaceUp(bool face_up)
{
    if (this->face_up != face_up) {
        this->face_up = face_up;
        emit state_changed();
    }
}

int Player::getMaxCards(const QString &except) const
{
    int origin = Sanguosha->correctMaxCards(this, true, except);
    if (origin == 0)
        origin = qMax(getHp(), 0);
    int rule = 0, total = 0, extra = 0;
    if (Config.MaxHpScheme == 3 && general2) {
        total = general->getMaxHp() + general2->getMaxHp();
        if (total % 2 != 0 && getMark("AwakenLostMaxHp") == 0)
            rule = 1;
    }
    extra += Sanguosha->correctMaxCards(this, false, except);

    return qMax(origin + rule + extra, 0);
}

QString Player::getKingdom() const
{
    if (kingdom.isEmpty() && general)
        return general->getKingdom();
    else
        return kingdom;
}

void Player::setKingdom(const QString &kingdom)
{
    if (this->kingdom != kingdom) {
        this->kingdom = kingdom;
        if (isHegemonyGameMode(ServerInfo.GameMode) && role == "careerist")
            return;
        if (isHegemonyGameMode(ServerInfo.GameMode)) {
            QStringList kingdoms = Sanguosha->getHegemonyKingdoms();
            if (!kingdoms.contains(kingdom))
                return;
        }
        emit kingdom_changed(kingdom);
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
    static QChar equip_flag('e');
    static QChar judging_flag('j');

    if (flags.contains("s") && flags.contains("h")) {
        if (!to->isKongcheng())
            return true;
    } else if (flags.contains("s")) {
        if (!to->getShownHandcards().isEmpty())
            return true;
    } else if (flags.contains("h")) {
        if ((to->getHandcardNum() - to->getShownHandcards().size()) > 0)
            return true;
    }
    if (flags.contains(judging_flag) && !to->getJudgingArea().isEmpty())
        return true;
    if (flags.contains(equip_flag)) {
        QSet<QString> Equips;
        if (to->getWeapon()) {
            Equips << "weapon";
        }
        if (to->getArmor()) {
            Equips << "armor";
        }
        if (to->getDefensiveHorse()) {
            Equips << "dh";
        }
        if (to->getOffensiveHorse()) {
            Equips << "oh";
        }
        if (to->getTreasure()) {
            Equips << "treasure";
        }

        if (reason == "sidou") {
            if (Equips.contains("weapon"))
                Equips.remove("weapon");
        }
        if (!Equips.isEmpty())
            return true;
    }
    return false;
}

bool Player::canDiscard(const Player *to, int card_id, const QString &reason) const
{
    if (reason == "sidou") {
        if (to->getWeapon() && card_id == to->getWeapon()->effectiveID())
            return false;
    }

    if (this == to) {
        if (isJilei(getRoomObject()->getCard(card_id)))
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
        const Card *trick = getRoomObject()->getCard(trick_id);
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
    if (this->chained != chained) {
        Sanguosha->playSystemAudioEffect("chained");
        this->chained = chained;
        emit state_changed();
    }
}

bool Player::isRemoved() const
{
    return removed;
}

void Player::setRemoved(bool removed)
{
    if (this->removed != removed) {
        this->removed = removed;
        emit removedChanged();
    }
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

    // Slash *newslash = new Slash(Card::NoSuit, 0);
    const Card *new_shash = getRoomObject()->cloneCard("Slash");
    // newslash->deleteLater();
#define THIS_SLASH (slash == nullptr ? new_shash : slash)
    if (isProhibited(other, THIS_SLASH, others)) {
        getRoomObject()->cardDeleting(new_shash);
        return false;
    }

    if (distance_limit) {
        bool res = distanceTo(other, rangefix) <= getAttackRange() + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, this, THIS_SLASH);
        getRoomObject()->cardDeleting(new_shash);
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
        if (treasure)
            count++;
    }
    if (include_judging)
        count += judging_area.length();
    return count;
}

IDSet Player::getPile(const QString &pile_name) const
{
    if (pile_name == "shown_card")
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
        if (pile.startsWith("&") || (pile == "wooden_ox" && hasTreasure("wooden_ox"))) {
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
        handlist.append("hand");
    foreach (const QString &pile, this->getPileNames()) {
        if (pile.startsWith("&") || pile.startsWith("^"))
            handlist.append(pile);
        else if (pile == "wooden_ox" && hasTreasure("wooden_ox"))
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
    return history.value("Slash", 0) + history.value("ThunderSlash", 0) + history.value("FireSlash", 0) + history.value("PowerSlash", 0) + history.value("IronSlash", 0)
        + history.value("LightSlash", 0);
}

int Player::getAnalepticCount() const
{
    return history.value("Analeptic", 0) + history.value("MagicAnaleptic", 0);
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
    if (skill_name == "shenbao") // prevent infinite recursion for skill "shenbao"
        return false;

    if (hasSkill("shenbao")) {
        foreach (const Player *p, getAliveSiblings()) {
            if (p->hasEquipSkill(skill_name))
                return true;
        }
    }

    if (weapon) {
        const Weapon *weaponc = dynamic_cast<const Weapon *>(weapon->face());
        if (Sanguosha->getSkill(weaponc) && Sanguosha->getSkill(weaponc)->objectName() == skill_name)
            return true;
    }
    if (armor) {
        const Armor *armorc = dynamic_cast<const Armor *>(armor->face());
        if (Sanguosha->getSkill(armorc) && Sanguosha->getSkill(armorc)->objectName() == skill_name)
            return true;
    }
    if (treasure) {
        const Treasure *treasurec = dynamic_cast<const Treasure *>(treasure->face());
        if (Sanguosha->getSkill(treasurec) && Sanguosha->getSkill(treasurec)->objectName() == skill_name)
            return true;
    }
    return false;
}

QSet<const TriggerSkill *> Player::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skillList;

    foreach (QString skill_name, skills.keys() + skills2.keys() + acquired_skills.values() + acquired_skills2.values()) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill && !hasEquipSkill(skill->objectName()))
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
        if (skill && (include_equip || !hasEquipSkill(skill->objectName())) && (!visible_only || skill->isVisible()))
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
    QString color = yellow ? "#FFFF33" : "#FF0080";
    QList<const Skill *> skillList = getVisibleSkillList();
    if (flag == "head")
        skillList = getHeadSkillList(true, true);
    else if (flag == "deputy")
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
        desc.replace("\n", "<br/>");
        description.append(QString("<font color=%1><b>%2</b>:</font> %3 <br/> <br/>").arg(color).arg(skill_name).arg(desc));
    }

    if (description.isEmpty())
        description = tr("<font color=%1>No skills</font>").arg(color);
    return description;
}

bool Player::isProhibited(const Player *to, const Card *card, const QList<const Player *> &others) const
{
    return Sanguosha->isProhibited(this, to, card, others);
}

bool Player::canSlashWithoutCrossbow(const Card *slash) const
{
    const Card *newslash = getRoomObject()->cloneCard("Slash");
#define THIS_SLASH (slash == NULL ? newslash : slash)
    int slash_count = getSlashCount();
    int valid_slash_count = 1;
    valid_slash_count += Sanguosha->correctCardTarget(TargetModSkill::Residue, this, THIS_SLASH);
    getRoomObject()->cardDeleting(newslash);
    return slash_count < valid_slash_count;
#undef THIS_SLASH
}

void Player::setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn)
{
    QStringList limit_type = limit_list.split(",");
    QString _pattern = pattern;
    if (!pattern.endsWith("$1") && !pattern.endsWith("$0")) {
        QString symb = single_turn ? "$1" : "$0";
        _pattern = _pattern + symb;
    }
    foreach (QString limit, limit_type) {
        Card::HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        card_limitation[method][reason] << _pattern;
    }
}

void Player::removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason)
{
    QStringList limit_type = limit_list.split(",");
    QString _pattern = pattern;
    if (!_pattern.endsWith("$1") && !_pattern.endsWith("$0"))
        _pattern = _pattern + "$0";
    foreach (QString limit, limit_type) {
        Card::HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        card_limitation[method][reason].removeOne(_pattern);
        if (card_limitation[method][reason].isEmpty() || _pattern.endsWith("$1") || clearReason)
            card_limitation[method].remove(reason);
    }
}

void Player::clearCardLimitation(bool single_turn)
{
    QList<Card::HandlingMethod> limit_type;
    limit_type << Card::MethodUse << Card::MethodResponse << Card::MethodDiscard << Card::MethodRecast << Card::MethodPindian;
    foreach (Card::HandlingMethod method, limit_type) {
        QMap<QString, QStringList> map = card_limitation[method];
        QMap<QString, QStringList>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            QString pattern = it.value().at(0);
            if (!single_turn || pattern.endsWith("$1")) {
                card_limitation[method].remove(it.key());
            }
        }
    }
}

bool Player::isCardLimited(const Card *card, Card::HandlingMethod method, bool isHandcard) const
{
    if (method == Card::MethodNone)
        return false;
    if (card->face()->type() == CardFace::TypeSkill && method == card->handleMethod()) {
        foreach (int card_id, card->subcards()) {
            const Card *c = getRoomObject()->getCard(card_id);
            QMap<QString, QStringList> map = card_limitation[method];
            QMap<QString, QStringList>::iterator it;
            for (it = map.begin(); it != map.end(); ++it) {
                QString pattern = it.value().at(0);
                QString _pattern = pattern.split("$").first();
                if (isHandcard)
                    _pattern.replace("hand", ".");
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
            QString _pattern = pattern.split("$").first();
            if (isHandcard)
                _pattern.replace("hand", ".");
            ExpPattern p(_pattern);
            if (p.match(this, card))
                return true;
        }
    }

    return false;
}

bool Player::isCardLimited(const QString &limit_list, const QString &reason) const
{
    QStringList limit_type = limit_list.split(",");
    foreach (QString limit, limit_type) {
        Card::HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        if (card_limitation[method].contains(reason))
            return true;
    }
    return false;
}

void Player::addQinggangTag(const Card *card)
{
    QStringList qinggang = tag["Qinggang"].toStringList();
    qinggang.append(card->toString());
    tag["Qinggang"] = QVariant::fromValue(qinggang);
}

void Player::removeQinggangTag(const Card *card)
{
    QStringList qinggang = tag["Qinggang"].toStringList();
    if (!qinggang.isEmpty()) {
        qinggang.removeOne(card->toString());
        tag["Qinggang"] = qinggang;
    }
}

void Player::copyFrom(Player *p)
{
    Player *b = this;
    Player *a = p;

    b->marks = QMap<QString, int>(a->marks);
    b->piles = QMap<QString, IDSet>(a->piles);
    b->acquired_skills = QSet<QString>(a->acquired_skills);
    b->acquired_skills2 = QSet<QString>(a->acquired_skills2);
    b->flags = QSet<QString>(a->flags);
    b->history = QHash<QString, int>(a->history);
    b->m_gender = a->m_gender;

    b->hp = a->hp;
    b->max_hp = a->max_hp;
    b->kingdom = a->kingdom;
    b->role = a->role;
    b->seat = a->seat;
    b->alive = a->alive;

    b->phase = a->phase;
    b->weapon = a->weapon;
    b->armor = a->armor;
    b->defensive_horse = a->defensive_horse;
    b->offensive_horse = a->offensive_horse;
    b->treasure = a->treasure;
    b->face_up = a->face_up;
    b->chained = a->chained;
    b->judging_area = QList<int>(a->judging_area);
    b->fixed_distance = QHash<const Player *, int>(a->fixed_distance);
    b->card_limitation = QMap<Card::HandlingMethod, QMap<QString, QStringList> >(a->card_limitation);

    b->tag = QVariantMap(a->tag);
}

QList<const Player *> Player::getSiblings() const
{
    QList<const Player *> siblings;
    if (parent()) {
        siblings = parent()->findChildren<const Player *>();
        siblings.removeOne(this);
    }
    return siblings;
}

QList<const Player *> Player::getAliveSiblings() const
{
    QList<const Player *> siblings = getSiblings();
    foreach (const Player *p, siblings) {
        if (!p->isAlive())
            siblings.removeOne(p);
    }
    return siblings;
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
        if (tr_skill && tr_skill->isGlobal() && !(skills.contains(tr_skill->objectName()) || skills2.contains(tr_skill->objectName())))
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
    foreach (const QString &skill, skill_name.split("|")) {
        bool checkpoint = true;
        foreach (const QString &sk, skill.split("+")) {
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
    if (flag.contains("h")) {
        foreach (const QString &skill, skills.keys()) {
            if (!Sanguosha->getSkill(skill)->canPreshow())
                continue;
            skills[skill] = preshowed;
        }
    }

    if (flag.contains("d")) {
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

bool Player::isHidden(const bool &head_general) const
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
    return general_showed || (general2 && general2_showed);
}

bool Player::hasShownAllGenerals() const
{
    return general_showed && (!general2 || general2_showed);
}

void Player::setGeneralShowed(bool showed)
{
    this->general_showed = showed;
    emit head_state_changed();
}

void Player::setGeneral2Showed(bool showed)
{
    this->general2_showed = showed;
    emit deputy_state_changed();
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

    if (role == "careerist" || player->role == "careerist")
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
        QString role = getRole();
        int i = 1;
        foreach (const Player *p, getSiblings()) {
            if (p->getRole() == role) {
                if (p->hasShownGeneral() && p->getRole() != "careerist")
                    ++i;
            }
        }
        if (i > (parent()->findChildren<const Player *>().length() / 2))
            return false;
        else if (role == player->getRole())
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
        if (p->getGeneral() && p->getGeneral()->isLord() && p->getKingdom() == kingdom)
            return p;
    }

    return nullptr;
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
    if (flags.contains('h')) {
        if (disableShow(true).contains(reason))
            return;
    }
    if (flags.contains('d')) {
        if (disableShow(false).contains(reason))
            return;
    }

    QString dis_str = flags + ',' + reason;
    disable_show << dis_str;
    emit disable_show_changed();
}

void Player::removeDisableShow(const QString &reason)
{
    QStringList remove_list;
    foreach (const QString &dis_str, disable_show) {
        QString dis_reason = dis_str.split(',').at(1);
        if (dis_reason == reason)
            remove_list << dis_str;
    }

    if (remove_list.isEmpty())
        return;

    foreach (const QString &to_remove, remove_list)
        disable_show.removeOne(to_remove);

    emit disable_show_changed();
}

QStringList Player::disableShow(bool head) const
{
    QChar head_flag = 'h';
    if (!head)
        head_flag = 'd';

    QStringList r;
    foreach (const QString &dis_str, disable_show) {
        QStringList dis_list = dis_str.split(',');
        if (dis_list.at(0).contains(head_flag))
            r << dis_list.at(1);
    }

    return r;
}

bool Player::canShowGeneral(const QString &flags) const
{
    bool head = true, deputy = true;
    foreach (const QString &dis_str, disable_show) {
        QStringList dis_list = dis_str.split(',');
        if (dis_list.at(0).contains("h"))
            head = false;
        if (dis_list.at(0).contains("d"))
            deputy = false;
    }
    if (flags.isEmpty())
        return head || deputy || hasShownOneGeneral();
    if (flags == "h")
        return head || hasShownGeneral();
    if (flags == "d")
        return deputy || hasShownGeneral2();
    if (flags == "hd")
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
        return !getGeneralName().contains("sujiang");
    else
        return getGeneral2() && !getGeneral2Name().contains("sujiang");
}
