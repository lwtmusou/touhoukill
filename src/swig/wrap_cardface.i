
%{

#include <optional>

namespace CardFaceLuaCall {

// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object

// CardFace

// also used by: isAvailable
// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<bool> targetFixed(lua_State *l, const Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace, CardFace.targetFixed }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace, CardFace.targetFixed }

    int call = lua_pcall(l, 3, 1, 0); // { cardFace.targetFixed() / error }

    if (call != LUA_OK)
        return std::nullopt;

    bool r = lua_toboolean(l, -1);
    return r;
}

// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<bool> targetsFeasible(lua_State *l, const QList<const Player *> &targets, const Player *Self, const Card *card)
{
    SWIG_NewPointerObj(l, &targets, SWIGTYPE_p_QListT_Player_const_p_t, 0); // { targets, CardFace, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, Self, SWIGTYPE_p_Player, 0); // { Self, targets, CardFace, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, Self, targets, CardFace, CardFace.targetsFeasible }

    int call = lua_pcall(l, 4, 1, 0); // { cardFace.targetFixed() / error }

    if (call != LUA_OK)
        return std::nullopt;

    bool r = lua_toboolean(l, -1);
    return r;
}

// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<int> targetFilter(lua_State *l, const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card)
{
    SWIG_NewPointerObj(l, &targets, SWIGTYPE_p_QListT_Player_const_p_t, 0); // { targets, CardFace, CardFace.targetFilter }
    SWIG_NewPointerObj(l, to_select, SWIGTYPE_p_Player, 0); // { to_select, targets, CardFace, CardFace.targetFilter }
    SWIG_NewPointerObj(l, Self, SWIGTYPE_p_Player, 0); // { Self, to_select, targets, CardFace, CardFace.targetFilter }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, Self, to_select, targets, CardFace, CardFace.targetFilter }

    int call = lua_pcall(l, 5, 1, 0); // { cardFace.targetsFeasible() / error }

    if (call != LUA_OK)
        return std::nullopt;

    int r = lua_tointeger(l, -1);
    return r;
}

// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<const Card *> validate(lua_State *l, const CardUseStruct &use)
{
    SWIG_NewPointerObj(l, &use, SWIGTYPE_p_CardUseStruct, 0); // { use, CardFace, CardFace.validate }

    int call = lua_pcall(l, 2, 1, 0); // { CardFace.validate() / error }

    if (call != LUA_OK)
        return std::nullopt;

    const Card *c = nullptr;

    int result = SWIG_ConvertPtr(l, -1, (void **)(&c), SWIGTYPE_p_Card, 0);
    if (!(SWIG_IsOK(result)))
        return std::nullopt;

    return c;
}

// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<const Card *> validateInResponse(lua_State *l, Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace, CardFace.validateInResponse }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace, CardFace.validateInResponse }

    int call = lua_pcall(l, 3, 1, 0); // { cardFace.validateInResponse() / error }

    if (call != LUA_OK)
        return std::nullopt;

    const Card *c = nullptr;

    int result = SWIG_ConvertPtr(l, -1, (void **)(&c), SWIGTYPE_p_Card, 0);
    if (!(SWIG_IsOK(result)))
        return std::nullopt;

    return c;
}

// also used by: doPreAction, onUse
// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
bool use(lua_State *l, RoomObject *room, const CardUseStruct &use)
{
    SWIG_NewPointerObj(l, room, SWIGTYPE_p_RoomObject, 0); // { room, CardFace, CardFace.use }
    SWIG_NewPointerObj(l, &use, SWIGTYPE_p_CardUseStruct, 0); // { use, room, CardFace, CardFace.use }

    int call = lua_pcall(l, 3, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK)
        return false;

    return true;
}

// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
bool onEffect(lua_State *l, const CardEffectStruct &effect)
{
    SWIG_NewPointerObj(l, &effect, SWIGTYPE_p_CardEffectStruct, 0); // { player, CardFace, CardFace.onEffect }

    int call = lua_pcall(l, 2, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK)
        return false;

    return true;
}

// [-2, +1, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
std::optional<bool> isCancelable(lua_State *l, const CardEffectStruct &effect)
{
    SWIG_NewPointerObj(l, &effect, SWIGTYPE_p_CardEffectStruct, 0); // { player, CardFace, CardFace.isCancelable }

    int call = lua_pcall(l, 2, 1, 0); // { cardFace.isCancelable() / error }

    if (call != LUA_OK)
        return std::nullopt;

    bool r = lua_toboolean(l, -1);
    return r;
}

// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
bool onNullified(lua_State *l, Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace, CardFace.onNullified }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace, CardFace.onNullified }

    int call = lua_pcall(l, 3, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK)
        return false;

    return true;
}

// EquipCard

// also used by: onUninstall, takeEffect
// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
bool onInstall(lua_State *l, Player *player)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace, EquipCard.onInstall }

    int call = lua_pcall(l, 2, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK)
        return false;

    return true;
}

// DelayedTrick
// [0, 0, -]
std::optional<JudgeStruct> judge(lua_State *l)
{
    JudgeStruct *arg2 = nullptr;

    if (!lua_isuserdata(l, 1))
        return std::nullopt;

    if (!SWIG_IsOK(SWIG_ConvertPtr(l, 1, (void **)&arg2, SWIGTYPE_p_JudgeStruct, 0)))
        return std::nullopt;

    return *arg2;
}

} // namespace CardFaceLuaCall

%}
