#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "engine.h"
#include "player.h"
#include "util.h"

// TODO: kill this
#include "room.h"

#include <QObject>
#include <QString>

using namespace QSanguosha;

class CardFacePrivate
{
public:
    CardFacePrivate()
        : target_fixed(false)
        , throw_when_using(true)
        , has_preact(false)
        , can_damage(false)
        , can_recover(false)
        , has_effectvalue(true)
        , default_method(MethodUse)
    {
    }

    bool target_fixed;
    bool throw_when_using;
    bool has_preact;
    bool can_damage;
    bool can_recover;
    bool has_effectvalue;

    HandlingMethod default_method;
};

CardFace::CardFace()
    : d(new CardFacePrivate)
{
}

CardFace::~CardFace()
{
    delete d;
}

QString CardFace::name() const
{
    return QString::fromUtf8(metaObject()->className());
}

QString CardFace::description() const
{
    return QString();
}

QString CardFace::commonEffectName() const
{
    return QString();
}

QString CardFace::effectName() const
{
    return QString();
}

bool CardFace::isKindOf(const char *cardType) const
{
    return inherits(cardType);
}

bool CardFace::matchType(const QString &pattern) const
{
    foreach (const QString &ptn, pattern.split(QStringLiteral("+"))) {
        if (typeName() == ptn || subTypeName() == ptn)
            return true;
    }
    return false;
}

bool CardFace::isNDTrick() const
{
    return false;
}

bool CardFace::canDamage() const
{
    return d->can_damage;
}

void CardFace::setCanDamage(bool can)
{
    d->can_damage = can;
}

bool CardFace::canRecover() const
{
    return d->can_recover;
}

void CardFace::setCanRecover(bool can)
{
    d->can_recover = can;
}

bool CardFace::hasEffectValue() const
{
    return d->has_effectvalue;
}

void CardFace::setHasEffectValue(bool can)
{
    d->has_effectvalue = can;
}

bool CardFace::throwWhenUsing() const
{
    return d->throw_when_using;
}

void CardFace::setThrowWhenUsing(bool can)
{
    d->throw_when_using = can;
}

bool CardFace::hasPreAction() const
{
    return d->has_preact;
}

void CardFace::setHasPreAction(bool can)
{
    d->has_preact = can;
}

HandlingMethod CardFace::defaultHandlingMethod() const
{
    return d->default_method;
}

void CardFace::setDefaultHandlingMethod(HandlingMethod can)
{
    d->default_method = can;
}

bool CardFace::targetFixed(const Player * /*unused*/, const Card * /*unused*/) const
{
    return d->target_fixed;
}

void CardFace::setTargetFixed(bool can)
{
    d->target_fixed = can;
}

bool CardFace::targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const
{
    if (targetFixed(Self, card))
        return true;
    else
        return !targets.isEmpty();
}

int CardFace::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card * /*unused*/) const
{
    return (targets.isEmpty() && to_select != Self) ? 1 : 0;
}

bool CardFace::isAvailable(const Player *player, const Card *card) const
{
    return !player->isCardLimited(card, card->handleMethod());
}

bool CardFace::ignoreCardValidity(const Player * /*unused*/) const
{
    return false;
}

const Card *CardFace::validate(const CardUseStruct &use) const
{
    return use.card;
}

const Card *CardFace::validateInResponse(Player * /*unused*/, const Card *original_card) const
{
    return original_card;
}

void CardFace::doPreAction(RoomObject * /*unused*/, const CardUseStruct & /*unused*/) const
{
}

void CardFace::onUse(RoomObject *room, const CardUseStruct &use) const
{
    CardUseStruct card_use = use;
    Player *player = card_use.from;

    // TODO
    // room->sortByActionOrder(card_use.to);

    QList<Player *> targets = card_use.to;
    // TODO
    // if (room->getMode() == QStringLiteral("06_3v3") && (isKindOf("AOE") || isKindOf("GlobalEffect")))
    //     room->reverseFor3v3(card_use.card, player, targets);
    card_use.to = targets;

    bool hidden = (type() == TypeSkill && !card_use.card->throwWhenUsing());
    LogMessage log;
    log.from = player;
    if (!targetFixed(card_use.from, card_use.card) || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = QStringLiteral("#UseCard");
    log.card_str = card_use.card->toString(hidden);
    RefactorProposal::fixme_cast<Room *>(room)->sendLog(log);

    IDSet used_cards;
    QList<CardsMoveStruct> moves;
    if (card_use.card->isVirtualCard())
        used_cards.unite(card_use.card->subcards());
    else
        used_cards.insert(card_use.card->effectiveID());

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = RefactorProposal::fixme_cast<Room *>(room)->getThread();
    Q_ASSERT(thread != nullptr);
    thread->trigger(PreCardUsed, data);
    card_use = data.value<CardUseStruct>();

    if (type() != TypeSkill) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, player->objectName(), QString(), card_use.card->skillName(), QString());
        if (card_use.to.size() == 1)
            reason.m_targetId = card_use.to.first()->objectName();

        reason.m_extraData = QVariant::fromValue(card_use.card);

        foreach (int id, used_cards) {
            CardsMoveStruct move(id, nullptr, PlaceTable, reason);
            moves.append(move);
        }
        RefactorProposal::fixme_cast<Room *>(room)->moveCardsAtomic(moves, true);
    } else {
        if (card_use.card->throwWhenUsing()) {
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->skillName(), QString());
            RefactorProposal::fixme_cast<Room *>(room)->moveCardTo(card_use.card, RefactorProposal::fixme_cast<ServerPlayer *>(player), nullptr, PlaceDiscardPile, reason, true);
        }
    }

    RefactorProposal::fixme_cast<ServerPlayer *>(player)->showHiddenSkill(card_use.card->showSkillName());

    thread->trigger(CardUsed, data);
    thread->trigger(CardFinished, data);
}

void CardFace::use(RoomObject *room, const CardUseStruct &use) const
{
    Player *source = use.from;

    QStringList nullified_list = use.nullified_list; // room->getTag(QStringLiteral("CardUseNullifiedList")).toStringList();
    bool all_nullified = nullified_list.contains(QStringLiteral("_ALL_TARGETS"));
    int magic_drank = 0;
    if (isNDTrick() && (source != nullptr) && source->getMark(QStringLiteral("magic_drank")) > 0)
        magic_drank = source->getMark(QStringLiteral("magic_drank"));

    foreach (Player *target, use.to) {
        CardEffectStruct effect;
        effect.card = use.card;
        effect.from = source;
        effect.to = target;
        effect.multiple = (use.to.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));
        if (use.card->hasFlag(QStringLiteral("mopao")))
            effect.effectValue.first() = effect.effectValue.first() + 1;
        if (use.card->hasFlag(QStringLiteral("mopao2")))
            effect.effectValue.last() = effect.effectValue.last() + 1;
        if (source != nullptr && source->getMark(QStringLiteral("kuangji_value")) > 0) {
            effect.effectValue.first() = effect.effectValue.first() + source->getMark(QStringLiteral("kuangji_value"));
            effect.effectValue.last() = effect.effectValue.last() + source->getMark(QStringLiteral("kuangji_value"));
            source->setMark(QStringLiteral("kuangji_value"), 0);
        }

        effect.effectValue.first() = effect.effectValue.first() + magic_drank;
        QVariantList players;
        for (int i = use.to.indexOf(target); i < use.to.length(); i++) {
            if (!nullified_list.contains(use.to.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(use.to.at(i)));
        }

        //room->setTag(QStringLiteral("targets") + use.card->toString(), QVariant::fromValue(players));

        // TODO: full card effect procedure //room->cardEffect(effect);
        effect.card->face()->onEffect(effect);
    }
    //room->removeTag(QStringLiteral("targets") + use.card->toString()); //for ai?
    if (source != nullptr && magic_drank > 0)
        source->setMark(QStringLiteral("magic_drank"), 0);

    if (RefactorProposal::fixme_cast<Room *>(room)->getCardPlace(use.card->effectiveID()) == PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source != nullptr ? source->objectName() : QString(), QString(), use.card->skillName(), QString());
        if (use.to.size() == 1)
            reason.m_targetId = use.to.first()->objectName();
        reason.m_extraData = QVariant::fromValue(use.card);
        Player *provider = nullptr;
        foreach (const QString &flag, use.card->flags()) {
            if (flag.startsWith(QStringLiteral("CardProvider_"))) {
                QStringList patterns = flag.split(QStringLiteral("_"));
                provider = RefactorProposal::fixme_cast<Room *>(room)->findPlayerByObjectName(patterns.at(1));
                break;
            }
        }

        Player *from = source;
        if (provider != nullptr)
            from = provider;
        RefactorProposal::fixme_cast<Room *>(room)->moveCardTo(use.card, RefactorProposal::fixme_cast<ServerPlayer *>(from), nullptr, PlaceDiscardPile, reason, true);
    }
}

void CardFace::onEffect(const CardEffectStruct & /*unused*/) const
{
}

bool CardFace::isCancelable(const CardEffectStruct & /*unused*/) const
{
    return false;
}

void CardFace::onNullified(Player * /*unused*/, const Card * /*unused*/) const
{
}

CardType BasicCard::type() const
{
    return TypeBasic;
}

QString BasicCard::typeName() const
{
    return QStringLiteral("basic");
}

CardType EquipCard::type() const
{
    return TypeEquip;
}

QString EquipCard::typeName() const
{
    return QStringLiteral("equip");
}

QString Weapon::subTypeName() const
{
    return QStringLiteral("weapon");
}

EquipLocation Weapon::location() const
{
    return WeaponLocation;
}

QString Armor::subTypeName() const
{
    return QStringLiteral("armor");
}

EquipLocation Armor::location() const
{
    return ArmorLocation;
}

QString DefensiveHorse::subTypeName() const
{
    return QStringLiteral("defensive_horse");
}

EquipLocation DefensiveHorse::location() const
{
    return DefensiveHorseLocation;
}

QString OffensiveHorse::subTypeName() const
{
    return QStringLiteral("offensive_horse");
}

EquipLocation OffensiveHorse::location() const
{
    return OffensiveHorseLocation;
}

QString Treasure::subTypeName() const
{
    return QStringLiteral("treasure");
}

EquipLocation Treasure::location() const
{
    return TreasureLocation;
}

CardType TrickCard::type() const
{
    return TypeTrick;
}

QString TrickCard::typeName() const
{
    return QStringLiteral("trick");
}

QString NonDelayedTrick::subTypeName() const
{
    return QStringLiteral("non_delayed_trick");
}

bool NonDelayedTrick::isNDTrick() const
{
    return true;
}

DelayedTrick::DelayedTrick()
    : j(nullptr)
{
}

QString DelayedTrick::subTypeName() const
{
    return QStringLiteral("delayed_trick");
}

JudgeStruct DelayedTrick::judge() const
{
    if (j == nullptr)
        return JudgeStruct();

    return *j;
}

CardType SkillCard::type() const
{
    return TypeSkill;
}

QString SkillCard::typeName() const
{
    return QStringLiteral("skill");
}

QString SkillCard::subTypeName() const
{
    return QStringLiteral("skill");
}

// TODO: find a suitable place for them
class SurrenderCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SurrenderCard();

    void onUse(RoomObject *room, const CardUseStruct &use) const override;
};

class CheatCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CheatCard();

    void onUse(RoomObject *room, const CardUseStruct &use) const override;
};

SurrenderCard::SurrenderCard()
{
    setTargetFixed(true);
    setDefaultHandlingMethod(MethodNone);
}

void SurrenderCard::onUse(RoomObject *room, const CardUseStruct &use) const
{
    RefactorProposal::fixme_cast<Room *>(room)->makeSurrender(RefactorProposal::fixme_cast<ServerPlayer *>(use.from));
}

CheatCard::CheatCard()
{
    setTargetFixed(true);
    setDefaultHandlingMethod(MethodNone);
}

void CheatCard::onUse(RoomObject *room, const CardUseStruct &use) const
{
    QString cheatString = use.card->userString();
    JsonDocument doc = JsonDocument::fromJson(cheatString.toUtf8().constData());
    if (doc.isValid())
        RefactorProposal::fixme_cast<Room *>(room)->cheat(RefactorProposal::fixme_cast<ServerPlayer *>(use.from), doc.toVariant());
}

#include "CardFace.moc"
