#include "aux-skills.h"
#include "CardFace.h"
#include "client.h"
#include "engine.h"
#include "package.h"
#include "util.h"

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

bool DiscardSkill::viewFilter(const QList<const Card *> &selected, const Card *card, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    if (selected.length() >= num)
        return false;

    if (!include_equip && Self->hasEquip(card))
        return false;

    if (is_discard && Self->isCardLimited(card, QSanguosha::MethodDiscard))
        return false;

    return true;
}

const Card *DiscardSkill::viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    if (cards.length() >= minnum) {
        auto *logic = Self->roomObject();
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
    this->pattern = Sanguosha->responsePattern(pattern);
    if (this->pattern == nullptr)
        this->pattern = Sanguosha->expPattern(pattern);
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

bool ResponseSkill::viewFilter(const Card *card, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    return matchPattern(Self, card);
}

const Card *ResponseSkill::viewAs(const Card *originalCard, const Player * /*Self*/, const QStringList &CurrentViewAsSkillChain) const
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
    : SkillCard(QStringLiteral("yiji"))
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

//int YijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player * /*Self*/, const Card *card) const
//{
//    return targets.isEmpty() && set.contains(to_select->objectName()) ? 1 : 0;
//}

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

bool YijiViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *card, const Player * /*Self*/, const QStringList &CurrentViewAsSkillChain) const
{
    return ids.contains(card->id()) && selected.length() < max_num;
}

const Card *YijiViewAsSkill::viewAs(const QList<const Card *> &cards, const Player *player, const QStringList &CurrentViewAsSkillChain) const
{
    if (cards.isEmpty() || cards.length() > max_num)
        return nullptr;

    auto *card = player->roomObject()->cloneSkillCard(QStringLiteral("YijiCard"));
    card->setHandleMethod(QSanguosha::MethodNone);

    card->clearSubcards();
    foreach (const Card *c, cards)
        card->addSubcard(c);
    return card;
}

// ------------------------------------------------

ChoosePlayerCard::ChoosePlayerCard()
    : SkillCard(QStringLiteral("choose_player"))
{
    setTargetFixed(false);
}

void ChoosePlayerCard::setPlayerNames(const QStringList &names)
{
    set = QSet<QString>(names.begin(), names.end());
}

//int ChoosePlayerCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player * /*Self*/, const Card * /*card*/) const
//{
//    return targets.isEmpty() && set.contains(to_select->objectName()) ? 1 : 0;
//}

// ------------------------------------------------

ChoosePlayerSkill::ChoosePlayerSkill()
    : ZeroCardViewAsSkill(QStringLiteral("choose_player"))
{
}

void ChoosePlayerSkill::setPlayerNames(const QStringList &names)
{
}

const Card *ChoosePlayerSkill::viewAs(const Player *player, const QStringList &CurrentViewAsSkillChain) const
{
    return player->roomObject()->cloneSkillCard(QStringLiteral("ChoosePlayerCard"));
}
