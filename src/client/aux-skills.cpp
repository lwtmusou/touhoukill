#include "aux-skills.h"
#include "CardFace.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"

DiscardSkill::DiscardSkill()
    : ViewAsSkill(QStringLiteral("discard"))
    , num(0)
    , include_equip(false)
    , is_discard(true)
{
    // card->setParent(this);
}

void DiscardSkill::setNum(int num)
{
    this->num = num;
}

void DiscardSkill::setMinNum(int minnum)
{
    this->minnum = minnum;
}

void DiscardSkill::setIncludeEquip(bool include_equip)
{
    this->include_equip = include_equip;
}

void DiscardSkill::setIsDiscard(bool is_discard)
{
    this->is_discard = is_discard;
}

bool DiscardSkill::viewFilter(const QList<const Card *> &selected, const Card *card, const Player *Self) const
{
    if (selected.length() >= num)
        return false;

    if (!include_equip && Self->hasEquip(card))
        return false;

    if (is_discard && Self->isCardLimited(card, QSanguosha::MethodDiscard))
        return false;

    return true;
}

const Card *DiscardSkill::viewAs(const QList<const Card *> &cards, const Player *Self) const
{
    if (cards.length() >= minnum) {
        auto *logic = Self->getRoomObject();
        auto *card = logic->cloneDummyCard();
        card->setHandleMethod(QSanguosha::MethodNone);
        card->clearSubcards();
        foreach (const Card *c, cards)
            card->addSubcard(c);
        return card;
    } else
        return nullptr;
}

// -------------------------------------------

ResponseSkill::ResponseSkill()
    : OneCardViewAsSkill(QStringLiteral("response-skill"))
{
    request = QSanguosha::MethodResponse;
}

void ResponseSkill::setPattern(const QString &pattern)
{
    this->pattern = Sanguosha->getPattern(pattern);
}

void ResponseSkill::setRequest(const QSanguosha::HandlingMethod request)
{
    this->request = request;
}

bool ResponseSkill::matchPattern(const Player *player, const Card *card) const
{
    if (request != QSanguosha::MethodNone && player->isCardLimited(card, request))
        return false;

    return (pattern != nullptr) && pattern->match(player, card);
}

bool ResponseSkill::viewFilter(const Card *card, const Player *Self) const
{
    return matchPattern(Self, card);
}

const Card *ResponseSkill::viewAs(const Card *originalCard, const Player * /*Self*/) const
{
    return originalCard;
}

// -------------------------------------------

ShowOrPindianSkill::ShowOrPindianSkill()
{
    setObjectName(QStringLiteral("showorpindian-skill"));
}

bool ShowOrPindianSkill::matchPattern(const Player *player, const Card *card) const
{
    return (pattern != nullptr) && pattern->match(player, card);
}

// -------------------------------------------

YijiCard::YijiCard()
{
    setTargetFixed(false);
    // mute = true;
    setThrowWhenUsing(false);
    // will_throw = false;
    // FIXME: How to pass the handling method to the card?
    // handling_method = QSanguosha::MethodNone;
}

void YijiCard::setPlayerNames(const QStringList &names)
{
    set = QSet<QString>(names.begin(), names.end());
}

int YijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player * /*Self*/, const Card *card) const
{
    return targets.isEmpty() && set.contains(to_select->objectName()) ? 1 : 0;
}

void YijiCard::use(Room *room, const CardUseStruct &use) const
{
    ServerPlayer *source = use.from;
    ServerPlayer *target = use.to.first();

    room->broadcastSkillInvoke(QStringLiteral("rende"));
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), QStringLiteral("nosrende"), QString());
    room->obtainCard(target, use.card, reason, false);

    int old_value = source->getMark(QStringLiteral("nosrende"));
    int new_value = old_value + use.card->subcards().size();
    room->setPlayerMark(source, QStringLiteral("nosrende"), new_value);

    if (old_value < 2 && new_value >= 2) {
        RecoverStruct recover;
        recover.card = use.card;
        recover.who = source;
        room->recover(source, recover);
    }
}

// -------------------------------------------

YijiViewAsSkill::YijiViewAsSkill()
    : ViewAsSkill(QStringLiteral("yiji"))
{
    // card->setParent(this);
}

void YijiViewAsSkill::setCards(const QString &card_str)
{
    QStringList cards = card_str.split(QStringLiteral("+"));
    ids = StringList2IntList(cards);
}

void YijiViewAsSkill::setMaxNum(int max_num)
{
    this->max_num = max_num;
}

void YijiViewAsSkill::setPlayerNames(const QStringList &names)
{
}

bool YijiViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *card, const Player * /*Self*/) const
{
    return ids.contains(card->id()) && selected.length() < max_num;
}

const Card *YijiViewAsSkill::viewAs(const QList<const Card *> &cards, const Player *player) const
{
    if (cards.isEmpty() || cards.length() > max_num)
        return nullptr;

    auto *card = player->getRoomObject()->cloneSkillCard(QStringLiteral("YijiCard"));
    card->setHandleMethod(QSanguosha::MethodNone);

    card->clearSubcards();
    foreach (const Card *c, cards)
        card->addSubcard(c);
    return card;
}

// ------------------------------------------------

ChoosePlayerCard::ChoosePlayerCard()
{
    setTargetFixed(false);
}

void ChoosePlayerCard::setPlayerNames(const QStringList &names)
{
    set = QSet<QString>(names.begin(), names.end());
}

int ChoosePlayerCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player * /*Self*/, const Card * /*card*/) const
{
    return targets.isEmpty() && set.contains(to_select->objectName()) ? 1 : 0;
}

// ------------------------------------------------

ChoosePlayerSkill::ChoosePlayerSkill()
    : ZeroCardViewAsSkill(QStringLiteral("choose_player"))
{
}

void ChoosePlayerSkill::setPlayerNames(const QStringList &names)
{
}

const Card *ChoosePlayerSkill::viewAs(const Player *player) const
{
    return player->getRoomObject()->cloneSkillCard(QStringLiteral("ChoosePlayerCard"));
}
