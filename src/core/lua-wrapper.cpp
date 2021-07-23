#include "lua-wrapper.h"

#include <QDebug>
#include <QFile>
#include <QThreadStorage>
#include <lua.hpp>

extern "C" {
    int luaopen_sgs(lua_State *l);
}

namespace {
// IMPORTANT! This should be updated when Lua updates.
// Currently we are cutting 'coroutine' lib out of standard Lua simply because it uses sjlj across calling stack, where it is not C++-exception-aware.
// Also we need to add our own 'sgs' lib to our preload modules to provide our functionality.
// Codes are copied from linit.c. MAKE SURE to update when Lua updates

constexpr const char *sgs_libname = "sgs";

const luaL_Reg sgs_libs[] = {{LUA_GNAME, luaopen_base},
                             {LUA_LOADLIBNAME, luaopen_package},
                             {LUA_TABLIBNAME, luaopen_table},
                             {LUA_IOLIBNAME, luaopen_io},
                             {LUA_OSLIBNAME, luaopen_os},
                             {LUA_STRLIBNAME, luaopen_string},
                             {LUA_MATHLIBNAME, luaopen_math},
                             {LUA_UTF8LIBNAME, luaopen_utf8},
                             {LUA_DBLIBNAME, luaopen_debug},
                             {sgs_libname, luaopen_sgs},
                             {nullptr, nullptr}};

void sgs_openlibs(lua_State *L)
{
    for (const luaL_Reg *lib = sgs_libs; lib->func != nullptr; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }
}

} // namespace

class LuaStatePrivate final
{
public:
    lua_State *l;

    LuaStatePrivate()
    {
        l = luaL_newstate();
        sgs_openlibs(l);

        // dostring the initial Lua code from qrc
        QFile f(QStringLiteral(":/luaInitialize.lua"));
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

        if (int luaRet = luaL_dostring(l, arr.constData()); luaRet != LUA_OK) {
            QString errorText = QString::fromUtf8(lua_tostring(l, -1));
            lua_pop(l, 1);
            if constexpr (errorDealingMethod == 1) {
                throw luaRet;
            } else if constexpr (errorDealingMethod == 2) {
                // removed
            } else if constexpr (errorDealingMethod == 3) {
                qDebug() << errorText;
            }
        }
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

class LuaMultiThreadedEnvironmentPrivate
{
public:
    QThreadStorage<LuaState *> stateStorage;
    QList<const Package *> packages;
    QList<const CardFace *> cardFaces;
    QList<const Skill *> skills;
};

LuaStatePointer LuaMultiThreadEnvironment::luaStateForCurrentThread()
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
    static QString v = QStringLiteral(LUA_RELEASE);
    return v;
}

const QString &LuaMultiThreadEnvironment::luaCopyright()
{
    static QString v = QStringLiteral(LUA_COPYRIGHT);
    return v;
}

LuaMultiThreadEnvironment::LuaMultiThreadEnvironment()
    : d(new LuaMultiThreadedEnvironmentPrivate)
{
    LuaState *firstLuaState = new LuaState;
    d->stateStorage.setLocalData(firstLuaState);

    LuaStatePointer firstLuaStatePtr = firstLuaState;
    // TODO: first LuaState is created, we should collect data from LuaState
    // notably Package, CardFace and Skill
    // Package maintains CardDescriptorList and GeneralList
    Q_UNIMPLEMENTED();
    Q_UNUSED(firstLuaStatePtr);
}

LuaMultiThreadEnvironment *LuaMultiThreadEnvironment::self()
{
    static LuaMultiThreadEnvironment instance;
    return &instance;
}

namespace BuiltinExtension {

// collect Extension name and checksum when called first time and cache it
// checksum should be in SHA256 or simular hashing algorithms
// Qt provides QCryptographicHash for this job, which we used to verify the auto-update in legacy TouhouSatsu
// Todo: find a way for trusted download of packages (PGP?)

QStringList names()
{
    Q_UNIMPLEMENTED();
    return QStringList();
}

bool verifyChecksum(const QString &)
{
    Q_UNIMPLEMENTED();
    return true;
}

// Should this be here? Maybe Engine should be responsible for this instead
// Exit program when a pure-server program runs, and disable connecting to any server (except for localhost) on main window
// Does nothing when debug is on
void disableConnectToServer()
{
    Q_UNIMPLEMENTED();
}

} // namespace BuiltinExtension
