#include "ai.h"
#include "aux-skills.h"
#include "engine.h"
#include "maneuvering.h"
#include "serverplayer.h"
#include "settings.h"
#include "standard.h"

#include <random>

AI::AI(ServerPlayer *player)
    : self(player)
{
    room = player->getRoom();
}

AI::~AI()
{
}

typedef QPair<QString, QString> RolePair;

struct RoleMapping : public QMap<RolePair, AI::Relation>
{
    void set(const QString &role1, const QString &role2, AI::Relation relation, bool bidirectional = false)
    {
        insert(qMakePair(role1, role2), relation);
        if (bidirectional)
            insert(qMakePair(role2, role1), relation);
    }

    AI::Relation get(const QString &role1, const QString &role2)
    {
        return value(qMakePair(role1, role2), AI::Neutrality);
    }
};

AI::Relation AI::GetRelation3v3(const ServerPlayer *a, const ServerPlayer *b)
{
    QChar c = a->getRole().at(0);
    if (b->getRole().startsWith(c))
        return Friend;
    else
        return Enemy;
}

AI::Relation AI::GetRelation(const ServerPlayer *a, const ServerPlayer *b)
{
    if (a == b)
        return Friend;
    static RoleMapping map, map_good, map_bad;
    if (map.isEmpty()) {
        map.set("lord", "lord", Friend);
        map.set("lord", "rebel", Enemy);
        map.set("lord", "loyalist", Friend);
        map.set("lord", "renegade", Neutrality);

        map.set("loyalist", "loyalist", Friend);
        map.set("loyalist", "lord", Friend);
        map.set("loyalist", "rebel", Enemy);
        map.set("loyalist", "renegade", Neutrality);

        map.set("rebel", "rebel", Friend);
        map.set("rebel", "lord", Enemy);
        map.set("rebel", "loyalist", Enemy);
        map.set("rebel", "renegade", Neutrality);

        map.set("renegade", "lord", Friend);
        map.set("renegade", "loyalist", Neutrality);
        map.set("renegade", "rebel", Neutrality);
        map.set("renegade", "renegade", Neutrality);

        map_good = map;
        map_good.set("renegade", "loyalist", Enemy, false);
        map_good.set("renegade", "lord", Neutrality, true);
        map_good.set("renegade", "rebel", Friend, false);

        map_bad = map;
        map_bad.set("renegade", "loyalist", Neutrality, true);
        map_bad.set("renegade", "rebel", Enemy, true);
    }

    if (a->aliveCount() == 2) {
        return Enemy;
    }

    QString roleA = a->getRole();
    QString roleB = b->getRole();

    Room *room = a->getRoom();

    int good = 0, bad = 0;
    QList<ServerPlayer *> players = room->getAlivePlayers();
    foreach (ServerPlayer *player, players) {
        switch (player->getRoleEnum()) {
        case Player::Lord:
        case Player::Loyalist:
            good++;
            break;
        case Player::Rebel:
            bad++;
            break;
        case Player::Renegade:
            good++;
            break;
        }
    }

    if (bad > good)
        return map_bad.get(roleA, roleB);
    else if (good > bad)
        return map_good.get(roleA, roleB);
    else
        return map.get(roleA, roleB);
}

AI::Relation AI::relationTo(const ServerPlayer *other) const
{
    if (self == other)
        return Friend;

    if (room->getMode() == "06_3v3" || room->getMode() == "06_XMode")
        return GetRelation3v3(self, other);

    return GetRelation(self, other);
}

bool AI::isFriend(const ServerPlayer *other) const
{
    return relationTo(other) == Friend;
}

bool AI::isEnemy(const ServerPlayer *other) const
{
    return relationTo(other) == Enemy;
}

QList<ServerPlayer *> AI::getEnemies() const
{
    QList<ServerPlayer *> players = room->getOtherPlayers(self);
    QList<ServerPlayer *> enemies;
    foreach (ServerPlayer *p, players)
        if (isEnemy(p))
            enemies << p;

    return enemies;
}

QList<ServerPlayer *> AI::getFriends() const
{
    QList<ServerPlayer *> players = room->getOtherPlayers(self);
    QList<ServerPlayer *> friends;
    foreach (ServerPlayer *p, players)
        if (isFriend(p))
            friends << p;

    return friends;
}

void AI::filterEvent(TriggerEvent, const QVariant &)
{
    // dummy
}

TrustAI::TrustAI(ServerPlayer *player)
    : AI(player)
{
    response_skill = new ResponseSkill;
    response_skill->setParent(this);
}

TrustAI::~TrustAI()
{
    delete response_skill;
}

void TrustAI::activate(CardUseStruct &card_use)
{
    QList<const Card *> cards = self->getHandcards();
    foreach (const Card *card, cards) {
        if (card->targetFixed(self)) {
            if (useCard(card)) {
                card_use.card = card;
                card_use.from = self;
                return;
            }
        }
    }
}

bool TrustAI::useCard(const Card *card)
{
    if (card->isKindOf("EquipCard")) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        switch (equip->location()) {
        case EquipCard::WeaponLocation: {
            WrappedCard *weapon = self->getWeapon();
            if (weapon == nullptr)
                return true;
            const Weapon *new_weapon = qobject_cast<const Weapon *>(equip);
            const Weapon *ole_weapon = qobject_cast<const Weapon *>(weapon->getRealCard());
            return new_weapon->getRange() > ole_weapon->getRange();
        }
        case EquipCard::ArmorLocation:
            return !self->getArmor();
        case EquipCard::OffensiveHorseLocation:
            return !self->getOffensiveHorse();
        case EquipCard::DefensiveHorseLocation:
            return !self->getDefensiveHorse();
        case EquipCard::TreasureLocation:
            return !self->getTreasure();
        default:
            return true;
        }
    }
    return false;
}

Card::Suit TrustAI::askForSuit(const QString &)
{
    return Card::AllSuits[QRandomGenerator::global()->generate() % 4];
}

QString TrustAI::askForKingdom()
{
    QString role;
    ServerPlayer *lord = room->getLord();
    QStringList kingdoms = Sanguosha->getKingdoms();
    kingdoms.removeOne("zhu");
    kingdoms.removeOne("touhougod");
    QString selfKingdom = self->getGeneral()->getKingdom();
    if (!lord)
        return kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());

    switch (self->getRoleEnum()) {
    case Player::Lord:
        role = kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());
        break;
    case Player::Renegade: {
        if (lord->getGeneral()->isLord() || self->hasSkill("hongfo"))
            role = lord->getKingdom();
        else if (lord->getGeneral2() && lord->getGeneral2()->isLord())
            role = lord->getGeneral2()->getKingdom();
        else
            role = kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());
        break;
    }
    case Player::Rebel: {
        if (self->hasSkill("hongfo")) {
            kingdoms.removeOne(lord->getKingdom());
            role = kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());
        } else if (lord->getGeneral()->isLord())
            role = lord->getKingdom();
        else
            role = kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());
        break;
    }
    case Player::Loyalist: {
        if (lord->getGeneral()->isLord() || self->hasSkill("hongfo"))
            role = lord->getKingdom();
        else if (lord->getGeneral2() && lord->getGeneral2()->isLord())
            role = lord->getGeneral2()->getKingdom();
        else {
            role = kingdoms.at(QRandomGenerator::global()->generate() % kingdoms.length());
        }
        break;
    }
    default:
        break;
    }
    if (kingdoms.contains(role))
        return role;
    else
        return "wai";
}

bool TrustAI::askForSkillInvoke(const QString &skill_name, const QVariant &)
{
    const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
    return skill != nullptr && skill->getFrequency() == Skill::Frequent;
}

QString TrustAI::askForChoice(const QString &, const QString &choice, const QVariant &)
{
    QStringList choices = choice.split("+");
    return choices.at(QRandomGenerator::global()->generate() % choices.length());
}

QList<int> TrustAI::askForDiscard(const QString &, int discard_num, int, bool optional, bool include_equip)
{
    QList<int> to_discard;
    if (optional)
        return to_discard;
    else
        return self->forceToDiscard(discard_num, include_equip, self->hasFlag("Global_AIDiscardExchanging"));
}

const Card *TrustAI::askForNullification(const Card *, ServerPlayer *, ServerPlayer *, bool)
{
    return nullptr;
}

int TrustAI::askForCardChosen(ServerPlayer *, const QString &, const QString &, Card::HandlingMethod)
{
    return -1;
}

const Card *TrustAI::askForCard(const QString &pattern, const QString &prompt, const QVariant &data)
{
    Q_UNUSED(prompt);
    Q_UNUSED(data);

    response_skill->setPattern(pattern);
    QList<const Card *> cards = self->getHandcards();
    foreach (const Card *card, cards)
        if (response_skill->matchPattern(self, card))
            return card;

    return nullptr;
}

QString TrustAI::askForUseCard(const QString &, const QString &, const Card::HandlingMethod)
{
    return ".";
}

int TrustAI::askForAG(const QList<int> &card_ids, bool refusable, const QString &)
{
    if (refusable)
        return -1;

    int r = QRandomGenerator::global()->generate() % card_ids.length();
    return card_ids.at(r);
}

const Card *TrustAI::askForCardShow(ServerPlayer *, const QString &)
{
    return self->getRandomHandCard();
}

const Card *TrustAI::askForPindian(ServerPlayer *requestor, const QString &)
{
    QList<const Card *> cards = self->getHandcards();
    std::sort(cards.begin(), cards.end(), Card::CompareByNumber);

    if (requestor != self && isFriend(requestor))
        return cards.first();
    else
        return cards.last();
}

ServerPlayer *TrustAI::askForPlayerChosen(const QList<ServerPlayer *> &targets, const QString &reason, bool optional)
{
    Q_UNUSED(reason);

    if (optional)
        return nullptr;

    int r = QRandomGenerator::global()->generate() % targets.length();
    return targets.at(r);
}

const Card *TrustAI::askForSinglePeach(ServerPlayer *dying)
{
    if (isFriend(dying)) {
        QList<const Card *> cards = self->getHandcards();
        foreach (const Card *card, cards) {
            if (card->isKindOf("Peach"))
                return card;
            if (card->isKindOf("Analeptic") && (dying == self || (dying->hasLordSkill("yanhui") && self->getKingdom() == "zhan")))
                return card;
        }
    }

    return nullptr;
}

ServerPlayer *TrustAI::askForYiji(const QList<int> &, const QString &, int &)
{
    return nullptr;
}

void TrustAI::askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type)
{
    Q_UNUSED(bottom);
    Q_UNUSED(guanxing_type);

    if (guanxing_type == Room::GuanxingDownOnly) {
        bottom = cards;
        up.clear();
    } else {
        up = cards;
        bottom.clear();
    }
}
