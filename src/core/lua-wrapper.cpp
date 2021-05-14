#include "lua-wrapper.h"

#include <QThreadStorage>
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

LuaState::LuaState()
    : d(new LuaStatePrivate)
{
}

LuaState::~LuaState()
{
    delete d;
}

lua_State *LuaState::data()
{
    return d->l;
}

const lua_State *LuaState::data() const
{
    return d->l;
}

class LuaMultiThreadedEnvironmentPrivate
{
public:
    QThreadStorage<LuaState *> stateStorage;
    QList<const Package *> packages;
    QList<const CardFace *> cardFaces;
    QList<const Skill *> skills;
};

LuaState *LuaMultiThreadEnvironment::luaStatePerThread()
{
    auto *storage = &(self()->d->stateStorage);

    if (!storage->hasLocalData())
        storage->setLocalData(new LuaState);

    return storage->localData();
}

const QList<const Package *> &LuaMultiThreadEnvironment::packages()
{
    return self()->d->packages;
}

const QList<const CardFace *> &LuaMultiThreadEnvironment::cardFaces()
{
    return self()->d->cardFaces;
}

const QList<const Skill *> &LuaMultiThreadEnvironment::skills()
{
    return self()->d->skills;
}

LuaMultiThreadEnvironment::LuaMultiThreadEnvironment()
    : d(new LuaMultiThreadedEnvironmentPrivate)
{
    LuaState *firstLuaState = luaStatePerThread();
    // TODO: first LuaState is created, we should collect data from LuaState
    // notably Package, CardFace and Skill
    // Package maintains CardDescriptorList and GeneralList
    Q_UNUSED(firstLuaState);
}

LuaMultiThreadEnvironment *LuaMultiThreadEnvironment::self()
{
    static LuaMultiThreadEnvironment instance;
    return &instance;
}
