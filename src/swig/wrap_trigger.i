
%{

#include <optional>

namespace TriggerLuaCall {

// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes the result (if any) or error object
bool record(lua_State *l, QSanguosha::TriggerEvent event, GameLogic *logic, QVariant &data)
{
    lua_pushnumber(l, static_cast<int>(event)); // { event, Trigger, Trigger.record }
    SWIG_NewPointerObj(l, logic, SWIGTYPE_p_GameLogic, 0); // { logic, event, Trigger, Trigger.record }
    SWIG_NewPointerObj(l, &data, SWIGTYPE_p_QVariant, 0); // { data, room, event, Trigger, Trigger.record }

    int call = lua_pcall(l, 4, 0, 0); // { error (if any) } / { }

    if (call != LUA_OK)
        return false;

    return true;
}

// [-2, 0, e]
// Assumes the corresponding function then the table itself are pushed on the top of stack, pops them and pushes only error object if error occurs
// return value from Lua is a table of TriggerDetail's. We need to convert it to QList for C++ to recognize them
bool triggerable(lua_State *l, QSanguosha::TriggerEvent event, GameLogic *logic, const QVariant &data, QList<TriggerDetail> &ret)
{
    lua_pushnumber(l, static_cast<int>(event)); // { event, Trigger, Trigger.triggerable }
    SWIG_NewPointerObj(l, logic, SWIGTYPE_p_GameLogic, 0); // { logic, event, Trigger, Trigger.triggerable }
    SWIG_NewPointerObj(l, &data, SWIGTYPE_p_QVariant, 0); // { data, logic, event, Trigger, Trigger.triggerable }

    int call = lua_pcall(l, 4, 1, 0); // { error (if any) } / table of TriggerDetail's

    if (call != LUA_OK)
        return false;

    bool warned = false;
    ret.clear();
    int length = luaL_len(l, -1);
    for (int i = 1; i <= length; ++i) {
        lua_geti(l, -1, i);
        TriggerDetail *arg1 = nullptr;
        if (SWIG_isptrtype(l, -1) && SWIG_IsOK(SWIG_ConvertPtr(l, -1, (void **)&arg1, SWIGTYPE_p_TriggerDetail, 0))) {
            ret << *arg1;
        } else {
            warned = true;
            lua_warning(l, QString(QStringLiteral("No. %1 of triggerable is not TriggerDetail, ignored.")).arg(i).toLocal8Bit().constData(), 1);
        }
    }
    if (warned)
        lua_warning(l, "", 0);

    return true;
}

std::optional<bool> trigger(lua_State *l, QSanguosha::TriggerEvent event, GameLogic *logic, const TriggerDetail &detail, QVariant &data);

std::optional<bool> skillTriggerCost(lua_State *l, QSanguosha::TriggerEvent event, GameLogic *logic, TriggerDetail &detail, QVariant &data);

} // namespace TriggerLuaCall

%}
