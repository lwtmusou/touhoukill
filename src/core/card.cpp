#include "card.h"
#include "client.h"
#include "engine.h"
#include "room.h"
#include "settings.h"
#include "standard.h"
#include "structs.h"

#include "CardFace.h"

#include <QFile>

const int Card::S_UNKNOWN_CARD_ID = -1;

const Card::Suit Card::AllSuits[4] = {Card::Spade, Card::Club, Card::Heart, Card::Diamond};

Card::Card(Suit suit, int number, bool target_fixed)
    : target_fixed(target_fixed)
    , mute(false)
    , will_throw(true)
    , has_preact(false)
    , can_recast(false)
    , m_suit(suit)
    , m_number(number)
    , m_id(-1)
    , can_damage(false)
    , can_recover(false)
    , has_effectvalue(true)
{
    handling_method = will_throw ? Card::MethodDiscard : Card::MethodUse;
}

QString Card::getSuitString() const
{
    return Suit2String(getSuit());
}

QString Card::Suit2String(Suit suit)
{
    switch (suit) {
    case Spade:
        return "spade";
    case Heart:
        return "heart";
    case Club:
        return "club";
    case Diamond:
        return "diamond";
    case NoSuitBlack:
        return "no_suit_black";
    case NoSuitRed:
        return "no_suit_red";
    default:
        return "no_suit";
    }
}

bool Card::isRed() const
{
    return getColor() == Red;
}

bool Card::isBlack() const
{
    return getColor() == Black;
}

int Card::getId() const
{
    return m_id;
}

void Card::setId(int id)
{
    m_id = id;
}

int Card::getEffectiveId() const
{
    if (isVirtualCard()) {
        if (subcards.isEmpty())
            return -1;
        else
            return subcards.first();
    } else
        return m_id;
}

int Card::getNumber() const
{
    if (m_number > 0)
        return m_number;
    if (isVirtualCard()) {
        if (subcardsLength() == 0)
            return 0;
        else {
            int num = 0;
            foreach (int id, subcards) {
                num += Sanguosha->currentRoomObject()->getCard(id)->getNumber();
            }
            return qMin(num, 13);
        }
    } else
        return m_number;
}

void Card::setNumber(int number)
{
    m_number = number;
}

QString Card::getNumberString() const
{
    int number = getNumber();
    if (isVirtualCard()) {
        if (subcardsLength() == 0 || subcardsLength() >= 2)
            number = 0;
    }
    if (number == 10)
        return "10";
    else {
        static const char *number_string = "-A23456789-JQK";
        return QString(number_string[number]);
    }
}

Card::Suit Card::getSuit() const
{
    if (m_suit != NoSuit && m_suit != SuitToBeDecided)
        return m_suit;
    if (isVirtualCard()) {
        if (subcardsLength() == 0)
            return NoSuit;
        else if (subcardsLength() == 1)
            return Sanguosha->currentRoomObject()->getCard(subcards.first())->getSuit();
        else {
            Color color = Colorless;
            foreach (int id, subcards) {
                Color color2 = Sanguosha->currentRoomObject()->getCard(id)->getColor();
                if (color == Colorless)
                    color = color2;
                else if (color != color2)
                    return NoSuit;
            }
            return (color == Red) ? NoSuitRed : NoSuitBlack;
        }
    } else
        return m_suit;
}

void Card::setSuit(Suit suit)
{
    m_suit = suit;
}

bool Card::sameColorWith(const Card *other) const
{
    return getColor() == other->getColor();
}

Card::Color Card::getColor() const
{
    switch (getSuit()) {
    case Spade:
    case Club:
    case NoSuitBlack:
        return Black;
    case Heart:
    case Diamond:
    case NoSuitRed:
        return Red;
    default:
        return Colorless;
    }
}

bool Card::isEquipped(const Player *Self) const
{
    return Self->hasEquip(this);
}

bool Card::matchTypeOrName(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    foreach (QString ptn, patterns)
        if (objectName() == ptn || getType() == ptn || getSubtype() == ptn)
            return true;
    return false;
}

bool Card::CompareByNumber(const Card *a, const Card *b)
{
    static Suit new_suits[] = {Spade, Heart, Club, Diamond, NoSuitBlack, NoSuitRed, NoSuit};
    Suit suit1 = new_suits[a->getSuit()];
    Suit suit2 = new_suits[b->getSuit()];

    if (a->m_number != b->m_number)
        return a->m_number < b->m_number;
    else
        return suit1 < suit2;
}

bool Card::CompareBySuit(const Card *a, const Card *b)
{
    static Suit new_suits[] = {Spade, Heart, Club, Diamond, NoSuitBlack, NoSuitRed, NoSuit};
    Suit suit1 = new_suits[a->getSuit()];
    Suit suit2 = new_suits[b->getSuit()];

    if (suit1 != suit2)
        return suit1 < suit2;
    else
        return a->m_number < b->m_number;
}

bool Card::CompareByType(const Card *a, const Card *b)
{
    int order1 = a->getTypeId();
    int order2 = b->getTypeId();
    if (order1 != order2)
        return order1 < order2;
    else {
        static QStringList basic;
        if (basic.isEmpty())
            basic << "slash"
                  << "ice_slash"
                  << "thunder_slash"
                  << "fire_slash"
                  << "jink"
                  << "peach"
                  << "analeptic"
                  << "light_jink"
                  << "chain_jink"
                  << "power_slash"
                  << "iron_slash"
                  << "light_slash"
                  << "magic_analeptic"
                  << "super_peach";
        switch (a->getTypeId()) {
        case TypeBasic: {
            foreach (QString object_name, basic) {
                if (a->objectName() == object_name) {
                    if (b->objectName() == object_name)
                        return CompareBySuit(a, b);
                    else
                        return true;
                }
                if (b->objectName() == object_name)
                    return false;
            }
            return CompareBySuit(a, b);
            break;
        }
        case TypeTrick: {
            if (a->objectName() == b->objectName())
                return CompareBySuit(a, b);
            else
                return a->objectName() < b->objectName();
            break;
        }
        case TypeEquip: {
            const EquipCard *eq_a = qobject_cast<const EquipCard *>(a->getRealCard());
            const EquipCard *eq_b = qobject_cast<const EquipCard *>(b->getRealCard());
            if (eq_a->location() == eq_b->location()) {
                if (eq_a->isKindOf("Weapon")) {
                    const Weapon *wep_a = qobject_cast<const Weapon *>(a->getRealCard());
                    const Weapon *wep_b = qobject_cast<const Weapon *>(b->getRealCard());
                    if (wep_a->getRange() == wep_b->getRange())
                        return CompareBySuit(a, b);
                    else
                        return wep_a->getRange() < wep_b->getRange();
                } else {
                    if (a->objectName() == b->objectName())
                        return CompareBySuit(a, b);
                    else
                        return a->objectName() < b->objectName();
                }
            } else {
                return eq_a->location() < eq_b->location();
            }
            break;
        }
        default:
            return CompareBySuit(a, b);
        }
    }
}

bool Card::isNDTrick() const
{
    return getTypeId() == TypeTrick && !isKindOf("DelayedTrick");
}

QString Card::getPackage() const
{
    if (parent())
        return parent()->objectName();
    else
        return QString();
}

QString Card::getFullName(bool include_suit) const
{
    QString name = getName();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(getSuitString());
        return QString("%1%2 %3").arg(suit_name).arg(getNumberString()).arg(name);
    } else
        return QString("%1 %2").arg(getNumberString()).arg(name);
}

QString Card::getLogName() const
{
    QString suit_char;
    QString number_string;

    switch (getSuit()) {
    case Spade:
    case Heart:
    case Club:
    case Diamond: {
        suit_char = QString("<img src='image/system/log/%1.png' height = 12/>").arg(getSuitString());
        break;
    }
    case NoSuitRed: {
        suit_char = tr("NoSuitRed");
        break;
    }
    case NoSuitBlack: {
        suit_char = tr("NoSuitBlack");
        break;
    }
    case NoSuit: {
        suit_char = tr("NoSuit");
        break;
    }
    default:
        break;
    }

    if (m_number > 0 && m_number <= 13)
        number_string = getNumberString();

    return QString("%1[%2%3]").arg(getName()).arg(suit_char).arg(number_string);
}

QString Card::getCommonEffectName() const
{
    return QString();
}

QString Card::getName() const
{
    return Sanguosha->translate(objectName());
}

QString Card::getSkillName(bool removePrefix) const
{
    if (m_skillName.startsWith("_") && removePrefix)
        return m_skillName.mid(1);
    else
        return m_skillName;
}

void Card::setSkillName(const QString &name)
{
    m_skillName = name;
}

QString Card::getDescription(bool yellow) const
{
    QString desc = Sanguosha->translate(":" + objectName());
    desc.replace("\n", "<br/>");
    return tr("<font color=%1><b>[%2]</b> %3</font>").arg(yellow ? "#FFFF33" : "#FF0080").arg(getName()).arg(desc);
}

QString Card::toString(bool hidden) const
{
    Q_UNUSED(hidden);
    if (!isVirtualCard())
        return QString::number(m_id);
    else
        return QString("%1:%2[%3:%4]=%5").arg(objectName()).arg(m_skillName).arg(getSuitString()).arg(getNumberString()).arg(subcardString());
}

QString Card::getEffectName() const
{
    QString name = objectName();
    for (int i = 0; i < name.length(); i++) {
        QChar ch = name[i];
        if (ch.isUpper()) {
            name[i] = ch.toLower();
            if (i == 0 || objectName() == "VSCrossbow")
                continue;
            name.insert(i, "_");
            break;
        }
    }
    return name;
}

QString Card::subcardString() const
{
    if (subcards.isEmpty())
        return ".";

    QStringList str;
    foreach (int subcard, subcards)
        str << QString::number(subcard);

    return str.join("+");
}

void Card::addSubcards(const QList<const Card *> &cards)
{
    foreach (const Card *card, cards)
        subcards.append(card->getId());
}

void Card::addSubcards(const QList<int> &subcards_list)
{
    subcards.append(subcards_list);
}

int Card::subcardsLength() const
{
    return subcards.length();
}

bool Card::isVirtualCard() const
{
    return m_id < 0;
}

const Card *Card::Parse(const QString &str, RoomObject *room)
{
    static QMap<QString, Card::Suit> suit_map;
    if (suit_map.isEmpty()) {
        suit_map.insert("spade", Card::Spade);
        suit_map.insert("club", Card::Club);
        suit_map.insert("heart", Card::Heart);
        suit_map.insert("diamond", Card::Diamond);
        suit_map.insert("no_suit_red", Card::NoSuitRed);
        suit_map.insert("no_suit_black", Card::NoSuitBlack);
        suit_map.insert("no_suit", Card::NoSuit);
    }

    if (str.startsWith(QChar('@'))) {
        // skill card
        QRegExp pattern("@(\\w+)=([^:]+)(:.+)?");
        QRegExp ex_pattern("@(\\w*)\\[(\\w+):(.+)\\]=([^:]+)(:.+)?");

        QStringList texts;
        QString card_name, card_suit, card_number;
        QStringList subcard_ids;
        QString subcard_str;
        QString user_string;

        if (pattern.exactMatch(str)) {
            texts = pattern.capturedTexts();
            card_name = texts.at(1);
            subcard_str = texts.at(2);
            user_string = texts.at(3);
        } else if (ex_pattern.exactMatch(str)) {
            texts = ex_pattern.capturedTexts();
            card_name = texts.at(1);
            card_suit = texts.at(2);
            card_number = texts.at(3);
            subcard_str = texts.at(4);
            user_string = texts.at(5);
        } else
            return nullptr;

        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        SkillCard *card = room->cloneSkillCard(card_name);

        if (card == nullptr)
            return nullptr;

        card->addSubcards(StringList2IntList(subcard_ids));

        // skill name
        // @todo: This is extremely dirty and would cause endless troubles.
        QString skillName = card->getSkillName();
        if (skillName.isNull()) {
            skillName = card_name.remove("Card").toLower();
            card->setSkillName(skillName);
        }
        if (!card_suit.isEmpty())
            card->setSuit(suit_map.value(card_suit, Card::NoSuit));

        if (!card_number.isEmpty()) {
            int number = 0;
            if (card_number == "A")
                number = 1;
            else if (card_number == "J")
                number = 11;
            else if (card_number == "Q")
                number = 12;
            else if (card_number == "K")
                number = 13;
            else
                number = card_number.toInt();

            card->setNumber(number);
        }

        if (!user_string.isEmpty()) {
            user_string.remove(0, 1);
            card->setUserString(user_string);
        }
        card->deleteLater();
        return card;
    } else if (str.startsWith(QChar('$'))) {
        QString copy = str;
        copy.remove(QChar('$'));
        QStringList card_strs = copy.split("+");
        DummyCard *dummy = new DummyCard(StringList2IntList(card_strs));
        dummy->deleteLater();
        return dummy;
    } else if (str.contains(QChar('='))) {
        QRegExp pattern("(\\w+):(\\w*)\\[(\\w+):(.+)\\]=(.+)");
        if (!pattern.exactMatch(str))
            return nullptr;

        QStringList texts = pattern.capturedTexts();
        QString card_name = texts.at(1);
        QString m_skillName = texts.at(2);
        QString suit_string = texts.at(3);
        QString number_string = texts.at(4);
        QString subcard_str = texts.at(5);
        QStringList subcard_ids;
        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        Suit suit = Card::NoSuit;
        DummyCard *dummy = new DummyCard(StringList2IntList(subcard_ids));
        if (suit_string == "to_be_decided")
            suit = dummy->getSuit();
        else
            suit = suit_map.value(suit_string, Card::NoSuit);
        delete dummy;

        int number = 0;
        if (number_string == "A")
            number = 1;
        else if (number_string == "J")
            number = 11;
        else if (number_string == "Q")
            number = 12;
        else if (number_string == "K")
            number = 13;
        else
            number = number_string.toInt();

        Card *card = room->cloneCard(card_name, suit, number);
        if (card == nullptr)
            return nullptr;

        card->addSubcards(StringList2IntList(subcard_ids));
        card->setSkillName(m_skillName);
        card->deleteLater();
        return card;
    } else {
        bool ok = false;
        int card_id = str.toInt(&ok);
        if (ok)
            return Sanguosha->currentRoomObject()->getCard(card_id)->getRealCard();
        else
            return nullptr;
    }
}

Card *Card::Clone(const Card *card)
{
    Card::Suit suit = card->getSuit();
    int number = card->getNumber();

    QObject *card_obj = nullptr;

    const QMetaObject *meta = card->metaObject();
    card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));

    if (card_obj) {
        Card *new_card = qobject_cast<Card *>(card_obj);
        if (new_card == nullptr) {
            delete card_obj;
            return nullptr;
        }
        new_card->setId(card->getId());
        new_card->setObjectName(card->objectName());
        new_card->addSubcard(card->getId());
        return new_card;
    } else
        return nullptr;
}

bool Card::targetFixed(const Player *Self) const
{
    bool ignore = (Self && Self->hasSkill("tianqu") && Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore && !(isKindOf("SkillCard") || isKindOf("AOE") || isKindOf("GlobalEffect")))
        return false;
    bool riyue_ignore = (Self && Self->hasSkill("riyue") && Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (riyue_ignore && !(isKindOf("SkillCard") || isKindOf("AOE") || isKindOf("GlobalEffect")) && ((canDamage() && isRed()) || (canRecover() && isBlack())))
        return false;

    if (Self && Self->hasFlag("Global_shehuoInvokerFailed"))
        return false;
    return target_fixed;
}

bool Card::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targetFixed(Self))
        return true;
    else
        return !targets.isEmpty();
}

bool Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

bool Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const
{
    bool canSelect = targetFilter(targets, to_select, Self);
    maxVotes = canSelect ? 1 : 0;
    return canSelect;
}

void Card::doPreAction(Room *, const CardUseStruct &) const
{
}

void Card::onUse(Room *room, const CardUseStruct &use) const
{
    CardUseStruct card_use = use;
    ServerPlayer *player = card_use.from;

    room->sortByActionOrder(card_use.to);

    QList<ServerPlayer *> targets = card_use.to;
    if (room->getMode() == "06_3v3" && (isKindOf("AOE") || isKindOf("GlobalEffect")))
        room->reverseFor3v3(this, player, targets);
    card_use.to = targets;

    bool hidden = (card_use.card->getTypeId() == TypeSkill && !card_use.card->willThrow());
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

    if (card_use.card->getTypeId() != TypeSkill) {
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
            room->moveCardTo(this, player, nullptr, Player::DiscardPile, reason, true);
        }
    }

    thread->trigger(CardUsed, room, data);
    thread->trigger(CardFinished, room, data);
}

void Card::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    int magic_drank = 0;
    if (isNDTrick() && source && source->getMark("magic_drank") > 0)
        magic_drank = source->getMark("magic_drank");

    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));
        if (hasFlag("mopao"))
            effect.effectValue.first() = effect.effectValue.first() + 1;
        if (hasFlag("mopao2"))
            effect.effectValue.last() = effect.effectValue.last() + 1;
        if (source->getMark("kuangji_value") > 0) {
            effect.effectValue.first() = effect.effectValue.first() + source->getMark("kuangji_value");
            effect.effectValue.last() = effect.effectValue.last() + source->getMark("kuangji_value");
            room->setPlayerMark(source, "kuangji_value", 0);
        }

        effect.effectValue.first() = effect.effectValue.first() + magic_drank;
        QVariantList players;
        for (int i = targets.indexOf(target); i < targets.length(); i++) {
            if (!nullified_list.contains(targets.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(targets.at(i)));
        }

        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }
    room->removeTag("targets" + this->toString()); //for ai?
    if (magic_drank > 0)
        room->setPlayerMark(source, "magic_drank", 0);

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), getSkillName(), QString());
        if (targets.size() == 1)
            reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue(this);
        ServerPlayer *provider = nullptr;
        foreach (QString flag, getFlags()) {
            if (flag.startsWith("CardProvider_")) {
                QStringList patterns = flag.split("_");
                provider = room->findPlayerByObjectName(patterns.at(1));
                break;
            }
        }

        ServerPlayer *from = source;
        if (provider != nullptr)
            from = provider;
        room->moveCardTo(this, from, nullptr, Player::DiscardPile, reason, true);
    }
}

void Card::onEffect(const CardEffectStruct &) const
{
}

bool Card::isCancelable(const CardEffectStruct &) const
{
    return false;
}

QString Card::showSkill() const
{
    return show_skill;
}

void Card::setShowSkill(const QString &skill_name)
{
    show_skill = skill_name;
}

void Card::addSubcard(int card_id)
{
    if (card_id < 0)
        qWarning("%s", qPrintable(tr("Subcard must not be virtual card!")));
    else
        subcards << card_id;
}

void Card::addSubcard(const Card *card)
{
    addSubcard(card->getEffectiveId());
}

QList<int> Card::getSubcards() const
{
    return subcards;
}

void Card::clearSubcards()
{
    subcards.clear();
}

bool Card::isAvailable(const Player *player) const
{
    return !player->isCardLimited(this, handling_method) || (can_recast && !player->isCardLimited(this, Card::MethodRecast));
}

bool Card::ignoreCardValidty(const Player *) const
{
    return false;
}

const Card *Card::validate(CardUseStruct &) const
{
    return this;
}

const Card *Card::validateInResponse(ServerPlayer *) const
{
    return this;
}

bool Card::isMute() const
{
    return mute;
}

bool Card::canDamage() const
{
#if 0
    if (getSkillName() == "xianshi" && Self) {
        QString selected_effect = Self->tag.value("xianshi", QString()).toString();
        if (selected_effect != nullptr) {
            Card *extracard = Sanguosha->cloneCard(selected_effect);
            extracard->deleteLater();
            if (extracard->canDamage())
                return true;
        }
    }
#endif
    return can_damage;
}

bool Card::canRecover() const
{
#if 0
    if (getSkillName() == "xianshi" && Self) {
        QString selected_effect = Self->tag.value("xianshi", QString()).toString();
        if (selected_effect != nullptr) {
            if (selected_effect.contains("analeptic"))
                return true;
            Card *extracard = Sanguosha->cloneCard(selected_effect);
            extracard->deleteLater();
            if (extracard->canRecover())
                return true;
        }
    }
#endif
    return can_recover;
}

bool Card::hasEffectValue() const
{
    return has_effectvalue;
}

bool Card::willThrow() const
{
    return will_throw;
}

bool Card::canRecast() const
{
    return can_recast;
}

void Card::setCanRecast(bool can)
{
    can_recast = can;
}

bool Card::hasPreAction() const
{
    return has_preact;
}

Card::HandlingMethod Card::getHandlingMethod() const
{
    return handling_method;
}

void Card::setFlags(const QString &flag) const
{
    static char symbol_c = '-';

    if (flag.isEmpty())
        return;
    else if (flag == ".")
        flags.clear();
    else if (flag.startsWith(symbol_c)) {
        QString copy = flag;
        copy.remove(symbol_c);
        flags.removeOne(copy);
    } else if (!flags.contains(flag))
        flags << flag;
}

bool Card::hasFlag(const QString &flag) const
{
    return flags.contains(flag);
}

void Card::clearFlags() const
{
    flags.clear();
}

// ---------   Skill card     ------------------

SkillCard::SkillCard()
    : Card(NoSuit, 0)
{
}

void SkillCard::setUserString(const QString &user_string)
{
    this->user_string = user_string;
}

QString SkillCard::getUserString() const
{
    return user_string;
}

QString SkillCard::getType() const
{
    return "skill_card";
}

QString SkillCard::getSubtype() const
{
    return "skill_card";
}

Card::CardType SkillCard::getTypeId() const
{
    return Card::TypeSkill;
}

QString SkillCard::toString(bool hidden) const
{
    QString str;
    if (!hidden)
        str = QString("@%1[%2:%3]=%4").arg(metaObject()->className()).arg(getSuitString()).arg(getNumberString()).arg(subcardString());
    else
        str = QString("@%1[no_suit:-]=.").arg(metaObject()->className());

    if (!user_string.isEmpty())
        return QString("%1:%2").arg(str).arg(user_string);
    else
        return str;
}

// ---------- Dummy card      -------------------

DummyCard::DummyCard()
    : SkillCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
    setObjectName("dummy");
}

DummyCard::DummyCard(const QList<int> &subcards)
    : SkillCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
    setObjectName("dummy");
    foreach (int id, subcards)
        this->subcards.append(id);
}

QString DummyCard::getType() const
{
    return "dummy_card";
}

QString DummyCard::getSubtype() const
{
    return "dummy_card";
}

QString DummyCard::toString(bool) const
{
    return "$" + subcardString();
}

ShowDistanceCard::ShowDistanceCard()
    : SkillCard()
{
    mute = true;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

const Card *ShowDistanceCard::validate(CardUseStruct &card_use) const
{
    QString c = toString().split(":").last();
    const Skill *skill = Sanguosha->getSkill(c);
    if (skill) {
        bool head = card_use.from->inHeadSkills(skill->objectName());
        card_use.from->showGeneral(head);
    }
    return nullptr;
}

ArraySummonCard::ArraySummonCard(const QString &name)
    : SkillCard()
{
    setObjectName(name);
    m_skillName = name;
    mute = true;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

const Card *ArraySummonCard::validate(CardUseStruct &card_use) const
{
    const BattleArraySkill *skill = qobject_cast<const BattleArraySkill *>(Sanguosha->getTriggerSkill(objectName()));
    if (skill) {
        card_use.from->showHiddenSkill(skill->objectName());
        skill->summonFriends(card_use.from);
    }
    return nullptr;
}

namespace RefactorProposal {

class CardPrivate
{
public:
    // basic information
    const CardFace *face; // functional model
    Card::Suit suit;
    Card::Number number;
    int id; // real card has id.

    QSet<int> sub_cards; // for real card this should be empty.

    // Skill name related
    QString skill_name;
    QString show_skill_name;

    // handling method
    Card::HandlingMethod handling_method;

    // property
    bool can_damage;
    bool can_recover;
    bool can_recast;
    bool has_effect_value;
    bool throw_when_using;

    // flags
    // Flag should be unique right?
    QSet<QString> flags;

    // UI (Why server side?)
    bool mute;
};

Card::Card(const CardFace *face, Suit suit, Number number, int id)
{
    d = new CardPrivate;
    // initialize d
    d->face = face;
    d->suit = suit;
    d->number = number;
    d->id = id;

    d->sub_cards.clear();
    d->handling_method = HandlingMethod::MethodNone;

    // initialized with the default info from the face.
    d->can_damage = face->canDamage();
    d->can_recover = face->canRecover();
    d->can_recast = false;
    d->has_effect_value = face->hasEffectValue();
    d->throw_when_using = face->throwWhenUsing();

    d->flags.clear();

    d->mute = false;
}

Card::~Card()
{
    delete d;
}

Card::Suit Card::suit() const
{
    if (d->suit != NoSuit && d->suit != SuitToBeDecided)
        return d->suit;
    if (isVirtualCard()) {
        if (d->sub_cards.size() == 0)
            return NoSuit;
        else if (d->sub_cards.size() == 0)
            return fixme_cast<Card::Suit>(Sanguosha->currentRoomObject()->getCard(*d->sub_cards.begin())->getSuit());
        else {
            Color color = Colorless;
            foreach (int id, d->sub_cards) {
                Color color2 = fixme_cast<Color>(Sanguosha->currentRoomObject()->getCard(id)->getColor());
                if (color == Colorless)
                    color = color2;
                else if (color != color2)
                    return NoSuit;
            }
            return (color == Red) ? NoSuitRed : NoSuitBlack;
        }
    } else
        return d->suit;
}

void Card::setSuit(Suit suit)
{
    d->suit = suit;
}

QString Card::suitString() const
{
    return SuitToString(d->suit);
}

bool Card::isRed() const
{
    return suit() == Heart || suit() == Diamond || suit() == NoSuitRed;
}

bool Card::isBlack() const
{
    return suit() == Spade || suit() == Club || suit() == NoSuitBlack;
}

Card::Color Card::color() const
{
    if (isRed())
        return Red;
    if (isBlack())
        return Black;
    return Colorless;
}

Card::Number Card::number() const
{
    return d->number;
}

void Card::setNumber(Number number)
{
    d->number = number;
}

QString Card::numberString() const
{
    Card::Number number = this->number();
    if (isVirtualCard()) {
        if (d->sub_cards.size() == 0 || d->sub_cards.size() >= 2)
            number = Number::NumberNA;
    }
    if (number == 10)
        return "10";
    else {
        static const char *number_string = "-A23456789-JQK";
        return QString(number_string[static_cast<int>(number)]);
    }
}

int Card::id() const
{
    return d->id;
}

int Card::effectiveID() const
{
    if (isVirtualCard()) {
        if (d->sub_cards.isEmpty())
            return -1;
        else
            return *d->sub_cards.begin();
    } else
        return d->id;
}

QString Card::name() const
{
    return d->face->name();
}

QString Card::fullName(bool include_suit) const
{
    QString name = this->name();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(suitString());
        return QString("%1%2 %3").arg(suit_name).arg(numberString()).arg(name);
    } else
        return QString("%1 %2").arg(numberString()).arg(name);
}

QString Card::logName() const
{
    QString suit_char;
    QString number_string;

    switch (suit()) {
    case Spade:
    case Heart:
    case Club:
    case Diamond: {
        suit_char = QString("<img src='image/system/log/%1.png' height = 12/>").arg(suitString());
        break;
    }
    case NoSuitRed: {
        // FIXME: use tr here will raise error since card does not inherits QObject.
        suit_char = Sanguosha->translate("NoSuitRed");
        break;
    }
    case NoSuitBlack: {
        suit_char = Sanguosha->translate("NoSuitRed");
        break;
    }
    case NoSuit: {
        suit_char = Sanguosha->translate("NoSuitRed");
        break;
    }
    default:
        break;
    }

    // FIXME: Should we compare the Number with int directly?
    if (d->number > 0 && d->number <= 13)
        number_string = numberString();

    return QString("%1[%2%3]").arg(name()).arg(suit_char).arg(number_string);
}

QString Card::skillName() const
{
    return d->skill_name;
}

void Card::setSkillName(const QString &skill_name)
{
    d->skill_name = skill_name;
}

QString Card::showSkillName() const
{
    return d->show_skill_name;
}

void Card::setShowSkillName(const QString &show_skill_name)
{
    d->show_skill_name = show_skill_name;
}

Card::HandlingMethod Card::handleMethod() const
{
    return d->handling_method;
}

void Card::setHandleMethod(Card::HandlingMethod method)
{
    d->handling_method = method;
}

bool Card::canDamage() const
{
    return d->can_damage;
}

void Card::setCanDamage(bool can_damage)
{
    d->can_damage = can_damage;
}

bool Card::canRecover() const
{
    return d->can_recover;
}

void Card::setCanRecover(bool can_recover)
{
    d->can_recover = can_recover;
}

bool Card::canRecast() const
{
    return d->can_recast;
}

void Card::setCanRecast(bool can_recast)
{
    d->can_recast = can_recast;
}

bool Card::hasEffectValue() const
{
    return d->has_effect_value;
}

void Card::setHasEffectValue(bool has_effect_value)
{
    d->has_effect_value = has_effect_value;
}

bool Card::throwWhenUsing() const
{
    return d->throw_when_using;
}

void Card::setThrowWhenUsing(bool throw_when_using)
{
    d->throw_when_using = throw_when_using;
}

const CardFace *Card::face() const
{
    return d->face;
}

void Card::setFace(const CardFace *face)
{
    d->face = face;
}

const QSet<QString> &Card::flags() const
{
    return d->flags;
}

void Card::addFlag(const QString &flag)
{
    d->flags.insert(flag);
}

void Card::addFlags(const QSet<QString> &flags)
{
    foreach (const QString &flag, flags) {
        d->flags.insert(flag);
    }
}

void Card::removeFlag(const QString &flag)
{
    d->flags.remove(flag);
}

void Card::removeFlag(const QSet<QString> &flags)
{
    foreach (const QString &flag, flags) {
        d->flags.remove(flag);
    }
}

void Card::clearFlags()
{
    d->flags.clear();
}

bool Card::hasFlag(const QString &flag) const
{
    return d->flags.contains(flag);
}

bool Card::isVirtualCard() const
{
    return id() < 0;
}

const QSet<int> &Card::subcards() const
{
    return d->sub_cards;
}

void Card::addSubcard(int card_id)
{
    Q_ASSERT(card_id >= 0);
    d->sub_cards.insert(card_id);
}

void Card::addSubcard(const Card *card)
{
    d->sub_cards.insert(card->effectiveID());
}

void Card::addSubcards(const QSet<int> &subcards)
{
    foreach (int id, subcards) {
        d->sub_cards.insert(id);
    }
}

void Card::clearSubcards()
{
    d->sub_cards.clear();
}

QString Card::subcardString() const
{
    if (subcards().isEmpty())
        return ".";

    QStringList str;
    foreach (int subcard, subcards())
        str << QString::number(subcard);

    return str.join("+");
}

bool Card::mute() const
{
    return d->mute;
}

void Card::setMute(bool mute)
{
    d->mute = mute;
}

QString Card::toString(bool hidden) const
{
    if (!isVirtualCard())
        return QString::number(id());
    else
        return QString("%1:%2[%3:%4]=%5").arg(name()).arg(skillName()).arg(suitString()).arg(numberString()).arg(subcardString());
}

Card::Card(CardPrivate *d)
{
    this->d = new CardPrivate(*d);
}

Card *Card::Clone(const Card *other)
{
    return new Card(other->d);
}

QString Card::SuitToString(Suit suit)
{
    switch (suit) {
    case Spade:
        return "spade";
    case Heart:
        return "heart";
    case Club:
        return "club";
    case Diamond:
        return "diamond";
    case NoSuitBlack:
        return "no_suit_black";
    case NoSuitRed:
        return "no_suit_red";
    default:
        return "no_suit";
    }
}

const Card *parse(const QString &str, RoomObject *room){
    // FIXME: Should we get rid of this function? 
    // We can replace this with a neat version by providing:
    // - CardFace name
    // - suit, number, or id?
    // - subcards id
    return nullptr;
}

};
