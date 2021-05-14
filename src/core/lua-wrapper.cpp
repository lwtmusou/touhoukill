#include "lua-wrapper.h"

#include <lua.hpp>

class LuaStatePrivate final
{
public:
    lua_State *l;

    LuaStatePrivate()
    {
        l = luaL_newstate();
        luaL_openlibs(l);
    }

    ~LuaStatePrivate()
    {
        lua_close(l);
    }
};

LuaState::LuaState(QObject *parent)
    : QObject(parent)
    , d(new LuaStatePrivate)
{
}

LuaState::~LuaState()
{
    delete d;
}
