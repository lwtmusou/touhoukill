#include "lua-wrapper.h"

#include <QDebug>
#include <QFile>
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

        // The sgs module (Although I don't want to use a single module anymore)
        // luaopen_sgs(l);
#if 0
        // dostring the initial Lua code from qrc
        QFile f(":/luaInitial.lua");
        f.open(QFile::ReadOnly);
        QByteArray arr = f.readAll();
        f.close();

        // luaInitial.lua in qrc must run initialization code from, e.g., /usr/share
        // Since luaL_doString itself calls lua_pcall, we can deal with error from it, but how?
        // 1. throw (will crash the game since caller is in constructor of LuaState)
        // 2. ---QMessageBox (will probably won't compile since this is Core where we should get rid of UI libs)--- removed because it breaks structure
        // 3. QDebug (Always builds and runs but hard to find its output on Windows by default, can be customized to use log file)
        // Temporary let's take method 3
        constexpr int errorDealingMethod = 3;

        int luaRet = luaL_dostring(l, arr.constData());
        if (luaRet != LUA_OK) {
            QString errorText = lua_tostring(l, -1);
            lua_pop(l, 1);
            if (errorDealingMethod == 1) {
                throw luaRet;
            } else if (errorDealingMethod == 2) {
                // removed
            } else if (errorDealingMethod == 3) {
                qDebug() << errorText;
            }
        }
#endif
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

const QString &LuaMultiThreadEnvironment::luaVersion()
{
    static QString v = LUA_RELEASE;
    return v;
}

const QString &LuaMultiThreadEnvironment::luaCopyright()
{
    static QString v = LUA_COPYRIGHT;
    return v;
}

LuaMultiThreadEnvironment::LuaMultiThreadEnvironment()
    : d(new LuaMultiThreadedEnvironmentPrivate)
{
    LuaState *firstLuaState = new LuaState;
    d->stateStorage.setLocalData(firstLuaState);

    // TODO: first LuaState is created, we should collect data from LuaState
    // notably Package, CardFace and Skill
    // Package maintains CardDescriptorList and GeneralList
}

LuaMultiThreadEnvironment *LuaMultiThreadEnvironment::self()
{
    static LuaMultiThreadEnvironment instance;
    return &instance;
}
