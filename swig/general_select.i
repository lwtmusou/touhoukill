
%{

#include "generalselector.h"

void GeneralSelector::initialize()
{
#define GETFUNCFROMLUASTATE(funcname)                    \
    do {                                                 \
        lua_getglobal(L, #funcname);                     \
        funcname##Func = luaL_ref(L, LUA_REGISTRYINDEX); \
        Q_ASSERT(funcname##Func != 0);                   \
    } while (false)

    GETFUNCFROMLUASTATE(initialize);
    GETFUNCFROMLUASTATE(selectFirst);
    GETFUNCFROMLUASTATE(selectSecond);
    GETFUNCFROMLUASTATE(select3v3);
    GETFUNCFROMLUASTATE(select1v1);
    GETFUNCFROMLUASTATE(arrange3v3);
    GETFUNCFROMLUASTATE(arrange1v1);

#undef GETFUNCFROMLUASTATE

    callLuaInitialize();
}

static void pushLuaFunction(lua_State *l, LuaFunction f)
{
    lua_rawgeti(l, LUA_REGISTRYINDEX, f);
}

void GeneralSelector::callLuaInitialize()
{
    pushLuaFunction(L, initializeFunc);
    int error = lua_pcall(L, 0, 0, 0);
    if (error)
        Error(L);
}

QString GeneralSelector::selectFirst(ServerPlayer *player, const QStringList &candidates)
{
    pushLuaFunction(L, selectFirstFunc);

    SWIG_NewPointerObj(L, player, SWIGTYPE_p_ServerPlayer, 0);

    lua_createtable(L, (&candidates)->length(), 0);

    for (int i = 0; i < (&candidates)->length(); i++) {
        QString str = (&candidates)->at(i);
        lua_pushstring(L, str.toUtf8());
        lua_rawseti(L, -2, i + 1);
    }

    int error = lua_pcall(L, 2, 1, 0);
    if (error)
        Error(L);
    else {
        QString s = lua_tostring(L, -1);
        lua_pop(L, 1);
        return s;
    }

    return candidates.first();
}

QString GeneralSelector::selectSecond(ServerPlayer *player, const QStringList &candidates)
{
    pushLuaFunction(L, selectSecondFunc);

    SWIG_NewPointerObj(L, player, SWIGTYPE_p_ServerPlayer, 0);

    lua_createtable(L, (&candidates)->length(), 0);

    for (int i = 0; i < (&candidates)->length(); i++) {
        QString str = (&candidates)->at(i);
        lua_pushstring(L, str.toUtf8());
        lua_rawseti(L, -2, i + 1);
    }

    int error = lua_pcall(L, 2, 1, 0);
    if (error)
        Error(L);
    else {
        QString s = lua_tostring(L, -1);
        lua_pop(L, 1);
        return s;
    }

    return candidates.first();
}

QString GeneralSelector::select3v3(ServerPlayer *player, const QStringList &candidates)
{
    pushLuaFunction(L, select3v3Func);

    SWIG_NewPointerObj(L, player, SWIGTYPE_p_ServerPlayer, 0);

    lua_createtable(L, (&candidates)->length(), 0);

    for (int i = 0; i < (&candidates)->length(); i++) {
        QString str = (&candidates)->at(i);
        lua_pushstring(L, str.toUtf8());
        lua_rawseti(L, -2, i + 1);
    }

    int error = lua_pcall(L, 2, 1, 0);
    if (error)
        Error(L);
    else {
        QString s = lua_tostring(L, -1);
        lua_pop(L, 1);
        return s;
    }

    return candidates.first();
}

QString GeneralSelector::select1v1(const QStringList &candidates)
{
    pushLuaFunction(L, select1v1Func);

    lua_createtable(L, (&candidates)->length(), 0);

    for (int i = 0; i < (&candidates)->length(); i++) {
        QString str = (&candidates)->at(i);
        lua_pushstring(L, str.toUtf8());
        lua_rawseti(L, -2, i + 1);
    }

    int error = lua_pcall(L, 1, 1, 0);
    if (error)
        Error(L);
    else {
        QString s = lua_tostring(L, -1);
        lua_pop(L, 1);
        return s;
    }

    return candidates.first();
}

QStringList GeneralSelector::arrange3v3(ServerPlayer *player)
{
    pushLuaFunction(L, arrange3v3Func);

    SWIG_NewPointerObj(L, player, SWIGTYPE_p_ServerPlayer, 0);

    int error = lua_pcall(L, 1, 1, 0);
    if (error)
        Error(L);
    else {
        QStringList l;
        for (size_t i = 0; i < lua_rawlen(L, -1); ++i) {
            lua_rawgeti(L, -1, i + 1);
            const char *elem = luaL_checkstring(L, -1);
            l << elem;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        return l;
    }

    return player->getSelected();
}

QStringList GeneralSelector::arrange1v1(ServerPlayer *player)
{
    pushLuaFunction(L, arrange1v1Func);

    SWIG_NewPointerObj(L, player, SWIGTYPE_p_ServerPlayer, 0);

    int error = lua_pcall(L, 1, 1, 0);
    if (error)
        Error(L);
    else {
        QStringList l;
        for (size_t i = 0; i < lua_rawlen(L, -1); ++i) {
            lua_rawgeti(L, -1, i + 1);
            const char *elem = luaL_checkstring(L, -1);
            l << elem;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        return l;
    }

    return player->getSelected();
}

%}