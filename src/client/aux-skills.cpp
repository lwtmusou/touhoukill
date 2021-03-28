#include "aux-skills.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "standard.h"

DiscardSkill::DiscardSkill()
    : ViewAsSkill("discard")
    , card(new DummyCard)
    , num(0)
    , include_equip(false)
    , is_discard(true)
{
    card->setParent(this);
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

    if (!include_equip && card->isEquipped(Self))
        return false;

    if (is_discard && Self->isCardLimited(card, Card::MethodDiscard))
        return false;

    return true;
}

const Card *DiscardSkill::viewAs(const QList<const Card *> &cards, const Player * /*Self*/) const
{
    if (cards.length() >= minnum) {
        card->clearSubcards();
        card->addSubcards(cards);
        return card;
    } else
        return nullptr;
}

// -------------------------------------------

ResponseSkill::ResponseSkill()
    : OneCardViewAsSkill("response-skill")
{
    request = Card::MethodResponse;
}

void ResponseSkill::setPattern(const QString &pattern)
{
    this->pattern = Sanguosha->getPattern(pattern);
}

void ResponseSkill::setRequest(const Card::HandlingMethod request)
{
    this->request = request;
}

bool ResponseSkill::matchPattern(const Player *player, const Card *card) const
{
    if (request != Card::MethodNone && player->isCardLimited(card, request))
        return false;

    return pattern && pattern->match(player, card);
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
    setObjectName("showorpindian-skill");
}

bool ShowOrPindianSkill::matchPattern(const Player *player, const Card *card) const
{
    return pattern && pattern->match(player, card);
}

// -------------------------------------------

class YijiCard : public SkillCard
{
public:
    YijiCard()
    {
        target_fixed = false;
        mute = true;
        will_throw = false;
        handling_method = Card::MethodNone;
    }

    void setPlayerNames(const QStringList &names)
    {
        set = QSet<QString>(names.begin(), names.end());
    }

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const override
    {
        return targets.isEmpty() && set.contains(to_select->objectName());
    }

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override
    {
        ServerPlayer *target = targets.first();

        room->broadcastSkillInvoke("rende");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
        room->obtainCard(target, this, reason, false);

        int old_value = source->getMark("nosrende");
        int new_value = old_value + subcards.length();
        room->setPlayerMark(source, "nosrende", new_value);

        if (old_value < 2 && new_value >= 2) {
            RecoverStruct recover;
            recover.card = this;
            recover.who = source;
            room->recover(source, recover);
        }
    }

private:
    QSet<QString> set;
};

YijiViewAsSkill::YijiViewAsSkill()
    : ViewAsSkill("yiji")
{
    card = new YijiCard;
    card->setParent(this);
}

void YijiViewAsSkill::setCards(const QString &card_str)
{
    QStringList cards = card_str.split("+");
    ids = StringList2IntList(cards);
}

void YijiViewAsSkill::setMaxNum(int max_num)
{
    this->max_num = max_num;
}

void YijiViewAsSkill::setPlayerNames(const QStringList &names)
{
    card->setPlayerNames(names);
}

bool YijiViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *card, const Player * /*Self*/) const
{
    return ids.contains(card->getId()) && selected.length() < max_num;
}

const Card *YijiViewAsSkill::viewAs(const QList<const Card *> &cards, const Player * /*Self*/) const
{
    if (cards.isEmpty() || cards.length() > max_num)
        return nullptr;

    card->clearSubcards();
    card->addSubcards(cards);
    return card;
}

// ------------------------------------------------

class ChoosePlayerCard : public DummyCard
{
public:
    ChoosePlayerCard()
    {
        target_fixed = false;
    }

    void setPlayerNames(const QStringList &names)
    {
        set = QSet<QString>(names.begin(), names.end());
    }

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const override
    {
        return targets.isEmpty() && set.contains(to_select->objectName());
    }

private:
    QSet<QString> set;
};

ChoosePlayerSkill::ChoosePlayerSkill()
    : ZeroCardViewAsSkill("choose_player")
{
    card = new ChoosePlayerCard;
    card->setParent(this);
}

void ChoosePlayerSkill::setPlayerNames(const QStringList &names)
{
    card->setPlayerNames(names);
}

const Card *ChoosePlayerSkill::viewAs(const Player * /*Self*/) const
{
    return card;
}
