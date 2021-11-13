#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "engine.h"
#include "lua-wrapper.h"
#include "player.h"
#include "util.h"

#include "lua.hpp"

#include <QObject>
#include <QString>

#include <optional>

using namespace QSanguosha;

#ifndef Q_DOC
class CardFacePrivate
{
public:
    CardFacePrivate()
    {
    }

    QString name;
    QString subTypeName;

    QStringList kind;

    std::optional<bool> target_fixed;
    std::optional<bool> has_preact;
    std::optional<bool> can_damage;
    std::optional<bool> can_recover;
    std::optional<bool> has_effectvalue;

    std::optional<HandlingMethod> default_method;
};

// somewhat not-even-be-a-method method for Lua calls which need to be done on SWIG side
// SWIG doesn't provide a binary-compatible way to export its constants
// These functions are implemented in wrap_cardface.i and generated in sgsLUA_wrap.cxx
namespace CardFaceLuaCall {
// CardFace
std::optional<bool> targetFixed(lua_State *l, const Player *player, const Card *card); // also used by: isAvailable
std::optional<bool> targetsFeasible(lua_State *l, const QList<const Player *> &targets, const Player *Self, const Card *card);
std::optional<int> targetFilter(lua_State *l, const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card);
std::optional<const Card *> validate(lua_State *l, const CardUseStruct &use);
std::optional<const Card *> validateInResponse(lua_State *l, Player *player, const Card *card);
bool use(lua_State *l, RoomObject *room, const CardUseStruct &use); // also used by: doPreAction, onUse
bool onEffect(lua_State *l, const CardEffectStruct &effect);
std::optional<bool> isCancelable(lua_State *l, const CardEffectStruct &effect);
bool onNullified(lua_State *l, Player *player, const Card *card);

// EquipCard
bool onInstall(lua_State *l, Player *player); // also used by: onUninstall, takeEffect

// DelayedTrick
std::optional<JudgeStruct> judge(lua_State *l);
} // namespace CardFaceLuaCall
#endif

// -- type (which is specified by desc.type using SecondTypeMask / ThirdTypeMask)
CardFace::CardFace(const QString &name)
    : d(new CardFacePrivate)
{
    d->name = name;
}

CardFace::~CardFace()
{
    delete d;
}

// -- name -> string
QString CardFace::name() const
{
    return d->name;
}

// -- subTypeName -> string
QString CardFace::subTypeName() const
{
    if (d->subTypeName.isEmpty()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return QString();

        do {
            int type = lua_getfield(l, -1, "subTypeName"); // { CardFace.subTypeName, CardFace }
            do {
                if (type != LUA_TSTRING)
                    break;
                d->subTypeName = QString::fromUtf8(lua_tostring(l, -1));
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->subTypeName;
}

// -- kind -> table (sequence) of strings
bool CardFace::isKindOf(const QString &cardType) const
{
    if (d->kind.isEmpty()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return cardType == QStringLiteral("CardFace");

        do {
            int type = lua_getfield(l, -1, "kind"); // { CardFace.kind, CardFace }
            do {
                if (type != LUA_TTABLE)
                    break;

                for (lua_pushnil(l); (bool)(lua_next(l, -2)); lua_pop(l, 1)) { // { v, k, CardFace.kind, CardFace }
                    type = lua_type(l, -1);
                    if (type != LUA_TSTRING)
                        continue;

                    d->kind << QString::fromUtf8(lua_tostring(l, -1));
                } // { CardFace.kind, CardFace }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (d->kind.isEmpty())
        return cardType == QStringLiteral("CardFace");

    return d->kind.contains(cardType);
}

// --  - canDamage = function() -> boolean
bool CardFace::canDamage() const
{
    bool r = false;

    if (!d->can_damage.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "canDamage"); // { CardFace.canDamage, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->can_damage = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { cardFace.canDamage() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_toboolean(l, -1);

                    // DO NOT STORE d->can_damage
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->can_damage.value_or(r);
}

void CardFace::setCanDamage(bool can)
{
    d->can_damage = can;
}

// --  - canRecover = function() -> boolean
bool CardFace::canRecover() const
{
    bool r = false;

    if (!d->can_recover.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "canRecover"); // { CardFace.canRecover, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->can_recover = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { cardFace.canRecover() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_toboolean(l, -1);

                    // DO NOT STORE d->can_recover
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->can_recover.value_or(r);
}

void CardFace::setCanRecover(bool can)
{
    d->can_recover = can;
}

// --  - hasEffectValue = function() -> boolean
bool CardFace::hasEffectValue() const
{
    bool r = false;

    if (!d->has_effectvalue.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "hasEffectValue"); // { CardFace.hasEffectValue, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->has_effectvalue = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { cardFace.hasEffectValue() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_toboolean(l, -1);

                    // DO NOT STORE d->has_effectvalue
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->has_effectvalue.value_or(r);
}

void CardFace::setHasEffectValue(bool can)
{
    d->has_effectvalue = can;
}

// --  - hasPreAction = function() -> boolean
bool CardFace::hasPreAction() const
{
    bool r = false;

    if (!d->has_preact.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "hasPreAction"); // { CardFace.hasPreAction, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->has_preact = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { cardFace.hasPreAction() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_toboolean(l, -1);

                    // DO NOT STORE d->has_preact
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->has_preact.value_or(r);
}

void CardFace::setHasPreAction(bool can)
{
    d->has_preact = can;
}

// --  - defaultHandlingMethod = function() -> QSanguosha_HandlingMethod
HandlingMethod CardFace::defaultHandlingMethod() const
{
    int r = static_cast<int>(MethodNone);

    if (!d->default_method.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return MethodNone;

        do {
            int type = lua_getfield(l, -1, "defaultHandlingMethod"); // { CardFace.defaultHandlingMethod, CardFace }
            do {
                if (type == LUA_TNUMBER) {
                    r = lua_tointeger(l, -1);
                    d->default_method = static_cast<HandlingMethod>(r);
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { cardFace.defaultHandlingMethod() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_tointeger(l, -1);

                    // DO NOT STORE d->default_method
                } else {
                    // neither integer nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->default_method.value_or(static_cast<HandlingMethod>(r));
}

void CardFace::setDefaultHandlingMethod(HandlingMethod can)
{
    d->default_method = can;
}

// --  - targetFixed = function(player, card) -> boolean
bool CardFace::targetFixed(const Player *player, const Card *card) const
{
    bool r = false;

    if (!d->target_fixed.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "targetFixed"); // { CardFace.targetFixed, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->target_fixed = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    std::optional<bool> call = CardFaceLuaCall::targetFixed(l, player, card); // { CardFace }
                    if (call.has_value())
                        r = call.value();
                    else {
                        // error
                        // since the stack top is the error object, we temporarily ignore it
                    }

                    // DO NOT STORE d->target_fixed
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->target_fixed.value_or(r);
}

void CardFace::setTargetFixed(bool can)
{
    d->target_fixed = can;
}

// --  - targetsFeasible - function(playerList, player, card) -> boolean
bool CardFace::targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const
{
    std::optional<bool> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "targetsFeasible"); // { CardFace.targetsFeasible, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::targetsFeasible(l, targets, Self, card); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    if (targetFixed(Self, card))
        return true;
    else
        return !targets.isEmpty();
}

// --  - targetFilter - function(playerList, player, player, card) -> integer
int CardFace::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const
{
    std::optional<int> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "targetFilter"); // { CardFace.targetFilter, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::targetFilter(l, targets, to_select, Self, card); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    return (targets.isEmpty() && to_select != Self) ? 1 : 0;
}

// --  - isAvailable - function(player, card) -> boolean
bool CardFace::isAvailable(const Player *player, const Card *card) const
{
    std::optional<bool> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "isAvailable"); // { CardFace.isAvailable, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::targetFixed(l, player, card); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    return !player->isCardLimited(card, card->handleMethod());
}

// --  - validate - function(cardUse) -> card
const Card *CardFace::validate(const CardUseStruct &use) const
{
    std::optional<const Card *> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "validate"); // { CardFace.validate, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::validate(l, use); // { returnValue / error, CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    return use.card;
}

// --  - validateInResponse - function(player, card) -> card
const Card *CardFace::validateInResponse(Player *player, const Card *original_card) const
{
    std::optional<const Card *> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "validateInResponse"); // { CardFace.validateInResponse, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::validateInResponse(l, player, original_card); // { returnValue / error, CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    return original_card;
}

// --  - doPreAction - function(room, cardUse)
void CardFace::doPreAction(RoomObject *room, const CardUseStruct &use) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "doPreAction"); // { CardFace.doPreAction, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::use(l, room, use); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    // do nothing
}

// --  - onUse - function(room, cardUse)
void CardFace::onUse(RoomObject *room, const CardUseStruct &use) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "onUse"); // { CardFace.onUse, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::use(l, room, use); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    defaultOnUse(room, use);
}

// --  - use - function(room, cardUse)
void CardFace::use(RoomObject *room, const CardUseStruct &use) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "use"); // { CardFace.use, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::use(l, room, use); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    defaultUse(room, use);
}

void CardFace::defaultOnUse(RoomObject *room, const CardUseStruct &use) const
{
#if 0
    CardUseStruct card_use = use;
    Player *player = card_use.from;

    room->sortPlayersByActionOrder(card_use.to);

    QList<Player *> targets = card_use.to;
    // TODO
    // if (room->getMode() == QStringLiteral("06_3v3") && (isKindOf("AOE") || isKindOf("GlobalEffect")))
    //     room->reverseFor3v3(card_use.card, player, targets);
    card_use.to = targets;

    LogMessage log;
    log.from = player;
    if (!targetFixed(card_use.from, card_use.card) || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = QStringLiteral("#UseCard");
    log.card_str = card_use.card->toString(false);
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

    CardMoveReason reason(CardMoveReason::S_REASON_USE, player->objectName(), QString(), card_use.card->skillName(), QString());
    if (card_use.to.size() == 1)
        reason.m_targetId = card_use.to.first()->objectName();

    reason.m_extraData = QVariant::fromValue(card_use.card);

    foreach (int id, used_cards) {
        CardsMoveStruct move(id, nullptr, PlaceTable, reason);
        moves.append(move);
    }
    RefactorProposal::fixme_cast<Room *>(room)->moveCardsAtomic(moves, true);

    RefactorProposal::fixme_cast<ServerPlayer *>(player)->showHiddenSkill(card_use.card->showSkillName());

    thread->trigger(CardUsed, data);
    thread->trigger(CardFinished, data);
#endif
}

void CardFace::defaultUse(RoomObject *room, const CardUseStruct &use) const
{
#if 0
    Player *source = use.from;

    QStringList nullified_list = use.nullified_list; // room->getTag(QStringLiteral("CardUseNullifiedList")).toStringList();
    bool all_nullified = nullified_list.contains(QStringLiteral("_ALL_TARGETS"));
    int magic_drank = 0;
    if (isNDTrick() && (source != nullptr) && source->mark(QStringLiteral("magic_drank")) > 0)
        magic_drank = source->mark(QStringLiteral("magic_drank"));

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
        if (source != nullptr && source->mark(QStringLiteral("kuangji_value")) > 0) {
            effect.effectValue.first() = effect.effectValue.first() + source->mark(QStringLiteral("kuangji_value"));
            effect.effectValue.last() = effect.effectValue.last() + source->mark(QStringLiteral("kuangji_value"));
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
                provider = room->findPlayer(patterns.at(1));
                break;
            }
        }

        Player *from = source;
        if (provider != nullptr)
            from = provider;
        RefactorProposal::fixme_cast<Room *>(room)->moveCardTo(use.card, RefactorProposal::fixme_cast<ServerPlayer *>(from), nullptr, PlaceDiscardPile, reason, true);
    }
#endif
}

// --  - onEffect(cardEffect)
void CardFace::onEffect(const CardEffectStruct &effect) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "onEffect"); // { CardFace.onEffect, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::onEffect(l, effect); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    // do nothing
}

// --  - isCancelable(cardEffect) -> boolean
bool CardFace::isCancelable(const CardEffectStruct &effect) const
{
    std::optional<bool> r;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "isCancelable"); // { CardFace.isCancelable, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    r = CardFaceLuaCall::isCancelable(l, effect); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (r.has_value())
        return r.value();

    // default
    return false;
}

// --  - onNullified(player, card)
void CardFace::onNullified(Player *player, const Card *card) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(d->name); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "onNullified"); // { CardFace.onNullified, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::onNullified(l, player, card); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    // do nothing
}

BasicCard::BasicCard(const QString &name)
    : CardFace(name)
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

EquipCard::EquipCard(const QString &name)
    : CardFace(name)
{
}

CardType EquipCard::type() const
{
    return TypeEquip;
}

QString EquipCard::typeName() const
{
    return QStringLiteral("equip");
}

#if 0
-- EquipCard has 5 subtypes which are corresponding to the 5 types of Equip card in Sanguosha.
-- The process of using EquipCard is totally different than using BasicCard and TrickCard. (Deal with it in C++)
-- Implement 5 subtypes separately and use this function as a backend
-- EquipCard has 2 optional function:
--  - onInstall
--  - onUninstall (for silverlion record, although this event seems able to record in a Trigger)
#endif
void EquipCard::onInstall(Player *player) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "onInstall"); // { EquipCard.onInstall, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::onInstall(l, player); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    defaultOnInstall(player);
}

void EquipCard::onUninstall(Player *player) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "onUninstall"); // { EquipCard.onInstall, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::onInstall(l, player); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    defaultOnUninstall(player);
}

void EquipCard::defaultOnInstall(Player * /*unused*/) const
{
#if 0
    // Shouldn't the attach skill logic be in GameLogic?

    Room *room = player->getRoom();

    const Skill *skill = Sanguosha->getSkill(this);
    if (skill) {
        if (skill->inherits("ViewAsSkill")) {
            room->attachSkillToPlayer(player, objectName());
        } else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            room->getThread()->addTriggerSkill(trigger_skill);

            if (trigger_skill->getViewAsSkill())
                room->attachSkillToPlayer(player, skill->objectName());
        }
    }
#endif
}

void EquipCard::defaultOnUninstall(Player * /*unused*/) const
{
#if 0
    // Shouldn't the detech skill logic be in GameLogic?

    Room *room = player->getRoom();
    const Skill *skill = Sanguosha->getSkill(this);

    if (skill && (skill->inherits("ViewAsSkill") || (skill->inherits("TriggerSkill") && qobject_cast<const TriggerSkill *>(skill)->getViewAsSkill())))
        room->detachSkillFromPlayer(player, objectName(), true);
#endif
}

#ifndef Q_DOC
class WeaponPrivate
{
public:
    std::optional<int> range;
};
#endif

Weapon::Weapon(const QString &name)
    : EquipCard(name)
    , d(new WeaponPrivate)
{
}

Weapon::~Weapon()
{
    delete d;
}

EquipLocation Weapon::location() const
{
    return WeaponLocation;
}

int Weapon::range() const
{
    if (!d->range.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
        if (!pushed)
            return 1;

        do {
            int type = lua_getfield(l, -1, "range"); // { Weapon.range, CardFace }
            do {
                if (type != LUA_TNUMBER)
                    break;
                d->range = lua_tointeger(l, -1);
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->range.value_or(1);
}

void Weapon::setRange(int r)
{
    d->range = r;
}

Armor::Armor(const QString &name)
    : EquipCard(name)
{
}

EquipLocation Armor::location() const
{
    return ArmorLocation;
}

DefensiveHorse::DefensiveHorse(const QString &name)
    : EquipCard(name)
{
}

EquipLocation DefensiveHorse::location() const
{
    return DefensiveHorseLocation;
}

OffensiveHorse::OffensiveHorse(const QString &name)
    : EquipCard(name)
{
}

EquipLocation OffensiveHorse::location() const
{
    return OffensiveHorseLocation;
}

Treasure::Treasure(const QString &name)
    : EquipCard(name)
{
}

EquipLocation Treasure::location() const
{
    return TreasureLocation;
}

TrickCard::TrickCard(const QString &name)
    : CardFace(name)
{
}

CardType TrickCard::type() const
{
    return TypeTrick;
}

QString TrickCard::typeName() const
{
    return QStringLiteral("trick");
}

NonDelayedTrick::NonDelayedTrick(const QString &name)
    : TrickCard(name)
{
}

#ifndef Q_DOC
class DelayedTrickPrivate
{
public:
    JudgeStruct *j;
    DelayedTrickPrivate()
        : j(nullptr)
    {
    }
};
#endif

DelayedTrick::DelayedTrick(const QString &name)
    : TrickCard(name)
    , d(new DelayedTrickPrivate)
{
}

DelayedTrick::~DelayedTrick()
{
    delete d->j;
    delete d;
}

void DelayedTrick::takeEffect(Player *target) const
{
    bool called = false;

    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
    if (pushed) {
        do {
            int type = lua_getfield(l, -1, "takeEffect"); // { DelayedTrick.takeEffect, CardFace }
            do {
                if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched in CardFaceLuaCall
                    called = CardFaceLuaCall::onInstall(l, target); // { CardFace }
                } else {
                    // not function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    if (called)
        return;

    // default
    // do nothing
}

void DelayedTrick::setJudge(const JudgeStruct &j)
{
    delete d->j;
    d->j = new JudgeStruct(j);
}

JudgeStruct DelayedTrick::judge() const
{
    if (d->j == nullptr) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
        if (!pushed)
            return JudgeStruct();

        do {
            lua_getfield(l, -1, "judge"); // { DelayedTrick.judge, CardFace }
            std::optional<JudgeStruct> j = CardFaceLuaCall::judge(l); // { CardFace }
            if (j.has_value())
                d->j = new JudgeStruct(j.value());
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->j == nullptr ? JudgeStruct() : JudgeStruct(*(d->j));
}

#ifndef Q_DOC
class SkillCardPrivate
{
public:
    std::optional<bool> throw_when_using;
};
#endif

SkillCard::SkillCard(const QString &name)
    : CardFace(name)
    , d(new SkillCardPrivate)

{
}

SkillCard::~SkillCard()
{
    delete d;
}

CardType SkillCard::type() const
{
    return TypeSkill;
}

QString SkillCard::typeName() const
{
    return QStringLiteral("skill");
}

void SkillCard::defaultOnUse(RoomObject *room, const CardUseStruct &_use) const
{
#if 0
    CardUseStruct card_use = _use;
    Player *player = card_use.from;

    room->sortPlayersByActionOrder(card_use.to);

    QList<Player *> targets = card_use.to;
    card_use.to = targets;

    LogMessage log;
    log.from = player;
    if (!targetFixed(card_use.from, card_use.card) || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = QStringLiteral("#UseCard");
    log.card_str = card_use.card->toString(!throwWhenUsing());
    RefactorProposal::fixme_cast<Room *>(room)->sendLog(log);

    if (throwWhenUsing()) {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->skillName(), QString());
        RefactorProposal::fixme_cast<Room *>(room)->moveCardTo(card_use.card, RefactorProposal::fixme_cast<ServerPlayer *>(player), nullptr, PlaceDiscardPile, reason, true);
    }

    RefactorProposal::fixme_cast<ServerPlayer *>(player)->showHiddenSkill(card_use.card->showSkillName());

    use(room, card_use);
#endif
}

void SkillCard::defaultUse(RoomObject * /*room*/, const CardUseStruct &use) const
{
    Player *source = use.from;
    foreach (Player *target, use.to) {
        CardEffectStruct effect;
        effect.card = use.card;
        effect.from = source;
        effect.to = target;
        onEffect(effect);
    }
}

bool SkillCard::throwWhenUsing() const
{
    bool r = true;

    if (!d->throw_when_using.has_value()) {
        LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
        bool pushed = l->pushCardFace(name()); // { CardFace(if successful) }
        if (!pushed)
            return false;

        do {
            int type = lua_getfield(l, -1, "throwWhenUsing"); // { SkillCard.throwWhenUsing, CardFace }
            do {
                if (type == LUA_TBOOLEAN) {
                    r = lua_toboolean(l, -1);
                    d->throw_when_using = r;
                } else if (type == LUA_TFUNCTION) {
                    // we should do the function call and return
                    // error should be catched here
                    int call = lua_pcall(l, 0, 1, 0); // { SkillCard.throwWhenUsing() / error, CardFace }
                    if (call == LUA_OK)
                        r = lua_toboolean(l, -1);

                    // DO NOT STORE d->throw_when_using
                } else {
                    // neither boolean nor function, ignored
                }
            } while (false);
            lua_pop(l, 1); // { CardFace }
        } while (false);
        lua_pop(l, 1); // { }
    }

    return d->throw_when_using.value_or(r);
}

void SkillCard::setThrowWhenUsing(bool can)
{
    d->throw_when_using = can;
}

#ifndef Q_DOC
// TODO: find a suitable place for them
class SurrenderCard : public SkillCard
{
public:
    SurrenderCard();

protected:
    void defaultOnUse(RoomObject *room, const CardUseStruct &use) const override;
};

class CheatCard : public SkillCard
{
public:
    CheatCard();

protected:
    void defaultOnUse(RoomObject *room, const CardUseStruct &use) const override;
};

SurrenderCard::SurrenderCard()
    : SkillCard(QStringLiteral("surrender"))
{
    setTargetFixed(true);
    setDefaultHandlingMethod(MethodNone);
}

void SurrenderCard::defaultOnUse(RoomObject *room, const CardUseStruct &use) const
{
#if 0
    RefactorProposal::fixme_cast<Room *>(room)->makeSurrender(RefactorProposal::fixme_cast<ServerPlayer *>(use.from));
#endif
}

CheatCard::CheatCard()
    : SkillCard(QStringLiteral("cheat"))
{
    setTargetFixed(true);
    setDefaultHandlingMethod(MethodNone);
}

void CheatCard::defaultOnUse(RoomObject *room, const CardUseStruct &use) const
{
#if 0
    QString cheatString = use.card->userString();
    JsonDocument doc = JsonDocument::fromJson(cheatString.toUtf8().constData());
    if (doc.isValid())
        RefactorProposal::fixme_cast<Room *>(room)->cheat(RefactorProposal::fixme_cast<ServerPlayer *>(use.from), doc.toVariant());
#endif
}
#endif
