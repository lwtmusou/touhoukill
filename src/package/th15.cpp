#include "th15.h"
#include "general.h"

#include "engine.h"
#include "skill.h"

YidanCard::YidanCard()
{
    will_throw = false;
}

bool YidanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    card->deleteLater();

    int slash_targets = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, card);
    if (targets.length() >= slash_targets)
        return false;
    if (!Self->isProhibited(to_select, card, targets)) {
        foreach (const Card *c, to_select->getEquips()) {
            if (card->getSuit() == c->getSuit())
                return true;
        }
        foreach (int id, to_select->getShownHandcards()) {
            if (card->getSuit() == Sanguosha->getCard(id)->getSuit())
                return true;
        }
        return !targets.isEmpty() && card->targetFilter(targets, to_select, Self);
    }
    return false;
}

bool YidanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    card->deleteLater();

    bool yidan = false;
    foreach (const Player *p, targets) {
        foreach (const Card *c, p->getEquips()) {
            if (card->getSuit() == c->getSuit()) {
                yidan = true;
                break;
            }
        }
        if (yidan)
            break;
        foreach (int id, p->getShownHandcards()) {
            if (card->getSuit() == Sanguosha->getCard(id)->getSuit()) {
                yidan = true;
                break;
            }
        }
        if (yidan)
            break;
    }
    return yidan && card->targetsFeasible(targets, Self);
}

const Card *YidanCard::validate(CardUseStruct &use) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    return card;
}

class YidanVS : public OneCardViewAsSkill
{
public:
    YidanVS()
        : OneCardViewAsSkill("yidan")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        return matchAvaliablePattern("slash", pattern);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            YidanCard *slash = new YidanCard;
            slash->addSubcard(originalCard);
            return slash;
        }
        return NULL;
    }
};

class Yidan : public TriggerSkill
{
public:
    Yidan()
        : TriggerSkill("yidan")
    {
        events << PreCardUsed;
        view_as_skill = new YidanVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName()) {
                if (use.m_addHistory) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        }
    }
};

class YidanProhibit : public ProhibitSkill
{
public:
    YidanProhibit()
        : ProhibitSkill("#yidan")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (to->hasSkill("yidan") && card->isKindOf("Slash")) {
            foreach (const Card *c, to->getEquips()) {
                if (card->getSuit() == c->getSuit())
                    return true;
            }
            foreach (int id, to->getShownHandcards()) {
                if (card->getSuit() == Sanguosha->getCard(id)->getSuit())
                    return true;
            }
        }
        return false;
    }
};

TH15Package::TH15Package()
    : Package("th15")
{
    General *junko = new General(this, "junko$", "gzz", 4, false, true, true);

    General *seiran = new General(this, "seiran", "gzz", 4, false);
    seiran->addSkill(new Yidan);
    seiran->addSkill(new YidanProhibit);
    related_skills.insertMulti("yidan", "#yidan");

    General *ringo = new General(this, "ringo", "gzz", 4, false, true, true);

    General *doremy = new General(this, "doremy", "gzz", 4, false, true, true);

    General *sagume = new General(this, "sagume", "gzz", 4, false, true, true);

    General *clownpiece = new General(this, "clownpiece", "gzz", 4, false, true, true);

    General *hecatia = new General(this, "hecatia", "gzz", 4, false, true, true);

    addMetaObject<YidanCard>();
}

ADD_PACKAGE(TH15)
