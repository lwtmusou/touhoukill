#include "CardFace.h"
#include "RoomObject.h"
#include "player.h"
#include "room.h"

#include <QObject>
#include <QString>

class CardFacePrivate
{
public:
    CardFacePrivate()
        : target_fixed(false)
        , will_throw(true)
        , has_preact(false)
        , can_recast(false)
        , can_damage(false)
        , can_recover(false)
        , has_effectvalue(true)
    {
    }

    bool target_fixed;
    bool will_throw;
    bool has_preact;
    bool can_recast;
    bool can_damage;
    bool can_recover;
    bool has_effectvalue;
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
    return staticMetaObject.className();
}

QString CardFace::description() const
{
    return QString();
}

QString CardFace::commmonEffectName() const
{
    return QString();
}

QString CardFace::effectName() const
{
    return QString();
}

bool CardFace::isKindOf(const char *cardType) const
{
    // TODO: return staticMetaObject.inherits(&(Sanguosha->getCardFace(cardType)->staticMetaObject)); // depends on Qt 5.7
    (void)cardType;
    return false;
}

bool CardFace::matchType(const QString &pattern) const
{
    foreach (const QString &ptn, pattern.split("+")) {
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

bool CardFace::canRecover() const
{
    return d->can_recover;
}

bool CardFace::canRecast() const
{
    return d->can_recast;
}

bool CardFace::hasEffectValue() const
{
    return d->has_effectvalue;
}

bool CardFace::willThrow() const
{
    return d->will_throw;
}

bool CardFace::hasPreAction() const
{
    return d->has_preact;
}

bool CardFace::targetFixed(const Player *, const Card *) const
{
    return d->target_fixed;
}

bool CardFace::targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const
{
    if (targetFixed(Self, card))
        return true;
    else
        return !targets.isEmpty();
}

bool CardFace::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *) const
{
    return targets.isEmpty() && to_select != Self;
}

bool CardFace::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card, int &maxVotes) const
{
    bool canSelect = targetFilter(targets, to_select, Self, card);
    maxVotes = canSelect ? 1 : 0;
    return canSelect;
}

bool CardFace::isAvailable(const Player *player, const Card *card) const
{
    return !player->isCardLimited(card, card->getHandlingMethod()) || (card->canRecast() && !player->isCardLimited(card, Card::MethodRecast));
}

bool CardFace::ignoreCardValidity(const Player *) const
{
    return false;
}

const Card *CardFace::validate(const CardUseStruct &use) const
{
    return use.card;
}

const Card *CardFace::validateInResponse(ServerPlayer *, const Card *original_card) const
{
    return original_card;
}

void CardFace::doPreAction(Room *, const CardUseStruct &) const
{
}

void CardFace::onUse(Room *room, const CardUseStruct &use) const
{
    CardUseStruct card_use = use;
    ServerPlayer *player = card_use.from;

    room->sortByActionOrder(card_use.to);

    QList<ServerPlayer *> targets = card_use.to;
    if (room->getMode() == "06_3v3" && (isKindOf("AOE") || isKindOf("GlobalEffect")))
        room->reverseFor3v3(card_use.card, player, targets);
    card_use.to = targets;

    bool hidden = (card_use.card->getTypeId() == Card::TypeSkill && !card_use.card->willThrow());
    LogMessage log;
    log.from = player;
    if (!card_use.card->targetFixed(card_use.from) || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString(hidden);
    room->sendLog(log);

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    if (card_use.card->isVirtualCard())
        used_cards.append(card_use.card->getSubcards());
    else
        used_cards << card_use.card->getEffectiveId();

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    Q_ASSERT(thread != nullptr);
    thread->trigger(PreCardUsed, room, data);
    card_use = data.value<CardUseStruct>();

    if (card_use.card->getTypeId() != Card::TypeSkill) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, player->objectName(), QString(), card_use.card->getSkillName(), QString());
        if (card_use.to.size() == 1)
            reason.m_targetId = card_use.to.first()->objectName();

        reason.m_extraData = QVariant::fromValue(card_use.card);

        foreach (int id, used_cards) {
            CardsMoveStruct move(id, nullptr, Player::PlaceTable, reason);
            moves.append(move);
        }
        room->moveCardsAtomic(moves, true);
        // show general
        player->showHiddenSkill(card_use.card->getSkillName());
    } else {
        const SkillCard *skill_card = qobject_cast<const SkillCard *>(card_use.card);
        // show general
        player->showHiddenSkill(skill_card->getSkillName());
        if (card_use.card->willThrow()) {
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->getSkillName(), QString());
            room->moveCardTo(card_use.card, player, nullptr, Player::DiscardPile, reason, true);
        }
    }

    thread->trigger(CardUsed, room, data);
    thread->trigger(CardFinished, room, data);
}

void CardFace::use(Room *room, const CardUseStruct &use) const
{
    ServerPlayer *source = use.from;

    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    int magic_drank = 0;
    if (isNDTrick() && source && source->getMark("magic_drank") > 0)
        magic_drank = source->getMark("magic_drank");

    foreach (ServerPlayer *target, use.to) {
        CardEffectStruct effect;
        effect.card = use.card;
        effect.from = source;
        effect.to = target;
        effect.multiple = (use.to.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));
        if (use.card->hasFlag("mopao"))
            effect.effectValue.first() = effect.effectValue.first() + 1;
        if (use.card->hasFlag("mopao2"))
            effect.effectValue.last() = effect.effectValue.last() + 1;
        if (source->getMark("kuangji_value") > 0) {
            effect.effectValue.first() = effect.effectValue.first() + source->getMark("kuangji_value");
            effect.effectValue.last() = effect.effectValue.last() + source->getMark("kuangji_value");
            room->setPlayerMark(source, "kuangji_value", 0);
        }

        effect.effectValue.first() = effect.effectValue.first() + magic_drank;
        QVariantList players;
        for (int i = use.to.indexOf(target); i < use.to.length(); i++) {
            if (!nullified_list.contains(use.to.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(use.to.at(i)));
        }

        room->setTag("targets" + use.card->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }
    room->removeTag("targets" + use.card->toString()); //for ai?
    if (magic_drank > 0)
        room->setPlayerMark(source, "magic_drank", 0);

    if (room->getCardPlace(use.card->getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), use.card->getSkillName(), QString());
        if (use.to.size() == 1)
            reason.m_targetId = use.to.first()->objectName();
        reason.m_extraData = QVariant::fromValue(use.card);
        ServerPlayer *provider = nullptr;
        foreach (QString flag, use.card->getFlags()) {
            if (flag.startsWith("CardProvider_")) {
                QStringList patterns = flag.split("_");
                provider = room->findPlayerByObjectName(patterns.at(1));
                break;
            }
        }

        ServerPlayer *from = source;
        if (provider != nullptr)
            from = provider;
        room->moveCardTo(use.card, from, nullptr, Player::DiscardPile, reason, true);
    }
}

void CardFace::onEffect(const CardEffectStruct &) const
{
}

bool CardFace::isCancelable(const CardEffectStruct &) const
{
    return false;
}

void CardFace::onNullified(ServerPlayer *target) const
{
}
