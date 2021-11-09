
%{

#include <optional>

namespace CardFaceLuaCall {

// All these functions have [-1, 0, -]
// All of them assume the corresponding function is push on the top of stack, and all of them pop it

// also used by: isAvailable
std::optional<bool> targetFixed(lua_State *l, const Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace.targetFixed }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace.targetFixed }

    int call = lua_pcall(l, 2, 1, 0); // { cardFace.targetFixed() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    bool r = lua_toboolean(l, -1);
    lua_pop(l, 1); // { }
    return r;
}

std::optional<bool> targetsFeasible(lua_State *l, const QList<const Player *> &targets, const Player *Self, const Card *card)
{
    SWIG_NewPointerObj(l, &targets, SWIGTYPE_p_QListT_Player_const_p_t, 0); // { targets, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, Self, SWIGTYPE_p_Player, 0); // { Self, targets, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, Self, targets, CardFace.targetsFeasible }

    int call = lua_pcall(l, 3, 1, 0); // { cardFace.targetsFeasible() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    bool r = lua_toboolean(l, -1);
    lua_pop(l, 1); // { }
    return r;
}

std::optional<int> targetFilter(lua_State *l, const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card)
{
    SWIG_NewPointerObj(l, &targets, SWIGTYPE_p_QListT_Player_const_p_t, 0); // { targets, CardFace.targetFilter }
    SWIG_NewPointerObj(l, to_select, SWIGTYPE_p_Player, 0); // { to_select, targets, CardFace.targetFilter }
    SWIG_NewPointerObj(l, Self, SWIGTYPE_p_Player, 0); // { Self, to_select, targets, CardFace.targetFilter }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, Self, to_select, targets, CardFace.targetFilter }

    int call = lua_pcall(l, 4, 1, 0); // { cardFace.targetsFeasible() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    int r = lua_tointeger(l, -1);
    lua_pop(l, 1); // { }
    return r;
}

std::optional<const Card *> validate(lua_State *l, const CardUseStruct &use)
{
    SWIG_NewPointerObj(l, &use, SWIGTYPE_p_CardUseStruct, 0); // { use, CardFace.validate }

    int call = lua_pcall(l, 1, 1, 0); // { CardFace.validate() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    const Card *c = nullptr;

    int result = SWIG_ConvertPtr(l, -1, (void **)(&c), SWIGTYPE_p_Card, 0);
    if (!(SWIG_IsOK(result))) {
        lua_pop(l, 1); // { }
        return std::nullopt;
    }

    lua_pop(l, 1); // { }
    return c;
}

std::optional<const Card *> validateInResponse(lua_State *l, Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace.validateInResponse }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace.validateInResponse }

    int call = lua_pcall(l, 2, 1, 0); // { cardFace.validateInResponse() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    const Card *c = nullptr;

    int result = SWIG_ConvertPtr(l, -1, (void **)(&c), SWIGTYPE_p_Card, 0);
    if (!(SWIG_IsOK(result))) {
        lua_pop(l, 1); // { }
        return std::nullopt;
    }

    lua_pop(l, 1); // { }
    return c;
}

// also used by: doPreAction, onUse
bool use(lua_State *l, RoomObject *room, const CardUseStruct &use)
{
    SWIG_NewPointerObj(l, room, SWIGTYPE_p_RoomObject, 0); // { room, CardFace.use }
    SWIG_NewPointerObj(l, &use, SWIGTYPE_p_CardUseStruct, 0); // { use, room, CardFace.use }

    int call = lua_pcall(l, 2, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { }
        return false;
    }

    return true;
}

bool onEffect(lua_State *l, const CardEffectStruct &effect)
{
    SWIG_NewPointerObj(l, &effect, SWIGTYPE_p_CardEffectStruct, 0); // { player, CardFace.onEffect }

    int call = lua_pcall(l, 1, 0, 0); // { cardFace.onEffect() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { }
        return false;
    }

    return true;
}

std::optional<bool> isCancelable(lua_State *l, const CardEffectStruct &effect)
{
    SWIG_NewPointerObj(l, &effect, SWIGTYPE_p_CardEffectStruct, 0); // { player, CardFace.isCancelable }

    int call = lua_pcall(l, 1, 1, 0); // { cardFace.isCancelable() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { } // need to pop the error object
        return std::nullopt;
    }

    bool r = lua_toboolean(l, -1);
    lua_pop(l, 1); // { }
    return r;
}

bool onNullified(lua_State *l, Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace.onNullified }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace.onNullified }

    int call = lua_pcall(l, 2, 0, 0); // { cardFace.onNullified() / error }

    if (call != LUA_OK) {
        lua_pop(l, 1); // { }
        return false;
    }

    return true;
}

}

%}
