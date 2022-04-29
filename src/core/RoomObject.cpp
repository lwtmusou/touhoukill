#include "RoomObject.h"
#include "CardFace.h"
#include "card.h"
#include "engine.h"
#include "exppattern.h"
#include "player.h"
#include "skill.h"
#include "util.h"

using namespace QSanguosha;

class RoomObjectPrivate
{
public:
    QHash<int, Card *> cards;
    QString currentCardUsePattern;
    CardUseReason currentCardUseReason;
    QList<Card *> clonedCards;

    QList<int> discardPile;

    QList<Player *> players;
    Player *current;
    QStringList seatInfo;

    // special skills
    QSet<const ProhibitSkill *> prohibit_skills;
    QSet<const DistanceSkill *> distance_skills;
    QSet<const TreatAsEquippingSkill *> viewhas_skills;
    QSet<const MaxCardsSkill *> maxcards_skills;
    QSet<const TargetModSkill *> targetmod_skills;
    QSet<const AttackRangeSkill *> attackrange_skills;
    QSet<const ViewAsSkill *> viewas_skills;

    RoomObjectPrivate()
        : current(nullptr)
    {
    }
};

RoomObject::RoomObject(QObject *parent)
    : QObject(parent)
    , d(new RoomObjectPrivate)
{
}

RoomObject::~RoomObject()
{
    foreach (Card *c, d->cards)
        delete c;

    foreach (Card *c, d->clonedCards)
        delete c;

    delete d;
}

QList<Player *> RoomObject::players(bool include_dead, bool include_removed)
{
    QList<Player *> ps = d->players;
    if (!include_dead) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isDead()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }
    if (!include_removed) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isRemoved()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }

    return ps;
}

QList<const Player *> RoomObject::players(bool include_dead, bool include_removed) const
{
    QList<Player *> ps = d->players;
    if (!include_dead) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isDead()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }
    if (!include_removed) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isRemoved()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }

    return NonConstList2ConstList(ps);
}

void RoomObject::registerPlayer(Player *player)
{
    d->players << player;
}

void RoomObject::unregisterPlayer(Player *player)
{
    d->players.removeAll(player);
}

void RoomObject::unregisterPlayer(const QString &objectName)
{
    for (auto it = d->players.begin(), lastIt = d->players.end(); it != d->players.end(); it == d->players.end() ? (it = d->players.begin()) : ++it) {
        if ((*it)->objectName() == objectName) {
            d->players.erase(it);
            it = lastIt;
        }

        lastIt = it;
    }
}

Player *RoomObject::findPlayer(const QString &objectName)
{
    foreach (Player *p, d->players)
        if (p->objectName() == objectName)
            return p;

    return nullptr;
}

const Player *RoomObject::findPlayer(const QString &objectName) const
{
    foreach (Player *p, d->players)
        if (p->objectName() == objectName)
            return p;

    return nullptr;
}

Player *RoomObject::current()
{
    return d->current;
}

const Player *RoomObject::current() const
{
    return d->current;
}

void RoomObject::setCurrent(Player *player)
{
    if (!(d->players.contains(player) || (player == nullptr)))
        return;

    d->current = player;

    if (player != nullptr) {
        QList<Player *> ps = d->players;
        while (ps.first() != player)
            ps << ps.takeFirst();

        d->players = ps;
    }
}

Player *RoomObject::findAdjecentPlayer(Player *player, bool next, bool include_dead, bool include_removed)
{
    QList<Player *> currentPlayers = players(include_dead, include_removed);

    int index = currentPlayers.indexOf(player);
    if (next) {
        index++;
        if (index == currentPlayers.length())
            index = 0;
    } else {
        index--;
        if (index < 0)
            index = currentPlayers.length() - 1;
    }

    return currentPlayers.at(index);
}

const Player *RoomObject::findAdjecentPlayer(const Player *player, bool next, bool include_dead, bool include_removed) const
{
    QList<const Player *> currentPlayers = players(include_dead, include_removed);

    int index = currentPlayers.indexOf(player);
    if (next) {
        index++;
        if (index == currentPlayers.length())
            index = 0;
    } else {
        index--;
        if (index < 0)
            index = currentPlayers.length() - 1;
    }

    return currentPlayers.at(index);
}

void RoomObject::arrangeSeat(const QStringList &_seatInfo)
{
    QList<Player *> ps = d->players;
    d->players.clear();

    QStringList seatInfo = _seatInfo;
    while (!seatInfo.isEmpty()) {
        QString first = seatInfo.takeFirst();
        for (auto it = ps.begin(); it != ps.end(); ++it) {
            if ((*it)->objectName() == first) {
                d->players << (*it);
                ps.erase(it);
                break;
            }
        }
    }

    if (d->current != nullptr) {
        ps = d->players;
        while (ps.first() != d->current)
            ps << ps.takeFirst();

        d->players = ps;
    }
}

// I don't think that there will be multiple players who share same seat, so use quicker std::sort instead of stable_sort
void RoomObject::sortPlayersByActionOrder(QList<Player *> &players) const
{
    std::sort(players.begin(), players.end(), [this](Player *a, Player *b) -> bool {
        return comparePlayerByActionOrder(a, b);
    });
}

void RoomObject::sortPlayersByActionOrder(QList<const Player *> &players) const
{
    std::sort(players.begin(), players.end(), [this](const Player *a, const Player *b) -> bool {
        return comparePlayerByActionOrder(a, b);
    });
}

bool RoomObject::comparePlayerByActionOrder(const Player *a, const Player *b) const
{
    QList<const Player *> ps = NonConstList2ConstList(d->players);
    return ps.indexOf(a) < ps.indexOf(b);
}

Card *RoomObject::getCard(int cardId)
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

const Card *RoomObject::getCard(int cardId) const
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

QSet<Card *> RoomObject::getCards()
{
    return List2Set(d->cards.values());
}

QSet<const Card *> RoomObject::getCards() const
{
    return List2Set(NonConstList2ConstList(d->cards.values()));
}

QString RoomObject::currentCardUsePattern() const
{
    return d->currentCardUsePattern;
}

void RoomObject::setCurrentCardUsePattern(const QString &newPattern)
{
    d->currentCardUsePattern = newPattern;
}

CardUseReason RoomObject::currentCardUseReason() const
{
    return d->currentCardUseReason;
}

void RoomObject::setCurrentCardUseReason(CardUseReason reason)
{
    d->currentCardUseReason = reason;
}

void RoomObject::resetCard(int cardId)
{
    // TODO_Fs: error check and sanity check
    delete d->cards[cardId];
    d->cards[cardId] = cloneCard(Sanguosha->cardDescriptor(cardId));

    // It does not depend on how we achieve the filter skill.
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetAllCards()
{
    foreach (Card *card, d->cards.values())
        delete card;
    d->cards.clear();

    int n = Sanguosha->cardCount();
    for (int i = 0; i < n; i++)
        d->cards[i] = cloneCard(Sanguosha->cardDescriptor(i));
}

QList<int> &RoomObject::discardPile()
{
    return d->discardPile;
}

const QList<int> &RoomObject::discardPile() const
{
    return d->discardPile;
}

Card *RoomObject::cloneCard(const Card *card)
{
    return cloneCard(card->face(), card->suit(), card->number());
}

Card *RoomObject::cloneCard(const QString &name, Suit suit, Number number)
{
    const CardFace *face = nullptr;

    if (!name.isEmpty()) {
        face = Sanguosha->cardFace(name);
        if (face == nullptr && name != QStringLiteral("DummyCard"))
            return nullptr;
    }
    return cloneCard(face, suit, number);
}

Card *RoomObject::cloneCard(const CardFace *cardFace, Suit suit, Number number)
{
    // Design change: dummy cards does not have CardFace
#if 0
    if (cardFace == nullptr) {
        qDebug() << "RoomObject::cloneCard - cardFace is nullptr";
        return nullptr;
    }
#endif

    Card *c = new Card(this, cardFace, suit, number);
    d->clonedCards << c;
    return c;
}

Card *RoomObject::cloneCard(const CardDescriptor &descriptor)
{
    return cloneCard(descriptor.face(), descriptor.suit, descriptor.number);
}

void RoomObject::cardDeleting(const Card *card)
{
    // const_cast is necessary for this function since card may be const
    // but for non-const Room, this Card instance is definitely mutable
    if (card != nullptr)
        d->clonedCards.removeAll(const_cast<Card *>(card));
    delete card;
}

QSet<const DistanceSkill *> RoomObject::getDistanceSkills() const
{
    return d->distance_skills;
}

const ViewAsSkill *RoomObject::getViewAsSkill(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->skill(skill_name);
    if (skill == nullptr)
        return nullptr;

    if (skill->inherits("ViewAsSkill"))
        return qobject_cast<const ViewAsSkill *>(skill);
    else
        return nullptr;
}

const ProhibitSkill *RoomObject::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    foreach (const ProhibitSkill *skill, d->prohibit_skills) {
        if (skill->isProhibited(from, to, card, others))
            return skill;
    }

    return nullptr;
}

const TreatAsEquippingSkill *RoomObject::treatAsEquipping(const Player *player, const QString &equipName, EquipLocation location) const
{
    foreach (const TreatAsEquippingSkill *skill, d->viewhas_skills) {
        if (skill->treatAs(player, equipName, location))
            return skill;
    }

    return nullptr;
}

int RoomObject::correctDistance(const Player *from, const Player *to) const
{
    int correct = 0;

    foreach (const DistanceSkill *skill, d->distance_skills) {
        correct += skill->getCorrect(from, to);
    }

    return correct;
}

int RoomObject::correctMaxCards(const Player *target, bool fixed, const QString &except) const
{
    int extra = 0;

    QStringList exceptlist = except.split(QStringLiteral("|"));

    foreach (const MaxCardsSkill *skill, d->maxcards_skills) {
        if (exceptlist.contains(skill->objectName()))
            continue;

        if (fixed) {
            int f = skill->getFixed(target);
            if (f >= 0)
                return f;
        } else {
            extra += skill->getExtra(target);
        }
    }

    return extra;
}

int RoomObject::correctCardTarget(TargetModType type, const Player *from, const Card *card) const
{
    int x = 0;
    if (type == ModResidue) {
        foreach (const TargetModSkill *skill, d->targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int residue = skill->getResidueNum(from, card);
                if (residue >= 998)
                    return residue;
                x += residue;
            }
        }
    } else if (type == ModDistance) {
        foreach (const TargetModSkill *skill, d->targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int distance_limit = skill->getDistanceLimit(from, card);
                if (distance_limit >= 998)
                    return distance_limit;
                x += distance_limit;
            }
        }
    } else if (type == ModTarget) {
        foreach (const TargetModSkill *skill, d->targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card) && from->mark(QStringLiteral("chuangshi_user")) == 0)
                x += skill->getExtraTargetNum(from, card);
        }
    }

    return x;
}

int RoomObject::correctAttackRange(const Player *target, bool include_weapon /* = true */, bool fixed /* = false */) const
{
    int extra = 0;

    foreach (const AttackRangeSkill *skill, d->attackrange_skills) {
        if (fixed) {
            int f = skill->getFixed(target, include_weapon);
            if (f >= 0)
                return f;
        } else {
            extra += skill->getExtra(target, include_weapon);
        }
    }

    return extra;
}

/**
 * @brief RoomObject::loadSkill
 * @param skill
 *
 * This is a temporary method for load a skill for this room.
 * Maybe refactored again during this refactor work.
 */
void RoomObject::loadSkill(const Skill *skill)
{
    if (skill->inherits("ProhibitSkill"))
        d->prohibit_skills << qobject_cast<const ProhibitSkill *>(skill);
    else if (skill->inherits("TreatAsEquippingSkill"))
        d->viewhas_skills << qobject_cast<const TreatAsEquippingSkill *>(skill);
    else if (skill->inherits("DistanceSkill"))
        d->distance_skills << qobject_cast<const DistanceSkill *>(skill);
    else if (skill->inherits("MaxCardsSkill"))
        d->maxcards_skills << qobject_cast<const MaxCardsSkill *>(skill);
    else if (skill->inherits("TargetModSkill"))
        d->targetmod_skills << qobject_cast<const TargetModSkill *>(skill);
    else if (skill->inherits("AttackRangeSkill"))
        d->attackrange_skills << qobject_cast<const AttackRangeSkill *>(skill);
    else if (skill->inherits("ViewAsSkill"))
        d->viewas_skills << qobject_cast<const ViewAsSkill *>(skill);
}

Card *RoomObject::cloneSkillCard(const QString &name)
{
    return cloneCard(name);
}

Card *RoomObject::cloneDummyCard()
{
    return cloneCard();
}

Card *RoomObject::cloneDummyCard(const IdSet &idSet)
{
    Card *c = cloneCard();
    if (c == nullptr)
        return nullptr;

    c->addSubcards(idSet);

    return c;
}

void QinggangSword::addQinggangTag(Player *p, const Card *card)
{
    QStringList qinggang = p->tag[QStringLiteral("Qinggang")].toStringList();
    qinggang.append(card->toString());
    p->tag[QStringLiteral("Qinggang")] = QVariant::fromValue(qinggang);
}

void QinggangSword::removeQinggangTag(Player *p, const Card *card)
{
    QStringList qinggang = p->tag[QStringLiteral("Qinggang")].toStringList();
    if (!qinggang.isEmpty()) {
        qinggang.removeOne(card->toString());
        p->tag[QStringLiteral("Qinggang")] = qinggang;
    }
}
