#include "lua-wrapper.h"
#include "engine.h"

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
// Codes are copied from linit.c. MAKE SURE to update these code when Lua updates.
// These codes are for Lua 5.3 and Lua 5.4 and only 5.4 will be tested.
// Lua 5.1 (probably the most popular version?) and Lua 5.2 (which is used in legacy QSanguosha v2) are out of support.

#if LUA_VERSION_NUM != 503 && LUA_VERSION_NUM != 504
#error Incompatible Lua version. QSanguosha requires Lua 5.3 or 5.4.
#endif

#if LUA_VERSION_NUM == 503
#warning You are using Lua 5.3 which is not fully supported. Consider install Lua 5.4 from your package manager, or use the builtin Lua 5.4.
#endif

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

#ifndef Q_DOC
class LuaStatePrivate final
{
public:
    lua_State *l;

    struct
    {
        int cardFacesTable;
        QHash<QString, int> cardFaces;
        int skillsTable;
        QHash<QString, int> skills;
        int packagesTable;
        QHash<QString, int> packages;
    } reg;

    LuaStatePrivate()
    {
        // since reg is unnamed, it should be initialized manually
        reg.cardFacesTable = LUA_NOREF;
        reg.skillsTable = LUA_NOREF;
        reg.packagesTable = LUA_NOREF;

        l = luaL_newstate();
        sgs_openlibs(l);

        // dostring the initial Lua code from qrc
        QFile f(QStringLiteral(":/luaInitialize.lua"));
        f.open(QFile::ReadOnly);
        QByteArray arr = f.readAll();
        f.close();

        // luaInitialize.lua in qrc may run initialization code from, e.g., /usr/share
        // Since luaL_doString itself calls lua_pcall, we can deal with error from it, but how?
        // 1. throw (will crash the game since caller is in constructor of LuaState)
        // 2. ---QMessageBox (will probably won't compile since this is Core where we should get rid of UI libs)--- removed because it breaks structure
        // 3. QDebug (Always builds and runs but hard to find its output on Windows by default, can be customized to use log file)
        // Temporary let's take method 3
        constexpr int errorDealingMethod = 3;

        int luaRet = luaL_loadbuffer(l, arr.constData(), arr.length(), "qrc:/luaInitialize.lua"); // { func(luaInitialize.lua) }
        if (luaRet != LUA_OK) {
            QString errorText = QString::fromUtf8(lua_tostring(l, -1));
            lua_pop(l, 1);
            if constexpr (errorDealingMethod == 1) {
                throw luaRet;
            } else if constexpr (errorDealingMethod == 2) {
                // removed
            } else if constexpr (errorDealingMethod == 3) {
                qDebug() << errorText;
            }

            return;
        }

        // I don't expect luaInitialize.lua return any value, so let's just set the number of return value to 0
        luaRet = lua_pcall(l, 0, 0, 0); // { }
        if (luaRet != LUA_OK) {
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

        // cache registry index for pushXxx use
        lua_newtable(l); // { (new table) }
        // since all setters pops the value from stack, we should push that again
        lua_setglobal(l, "sgs_registry"); // { }
        lua_getglobal(l, "sgs_registry"); // { sgs_registry }

        int type = lua_getglobal(l, "sgs_ex"); // { sgs_ex, sgs_registry }
        do {
            if (type != LUA_TTABLE) {
                qDebug() << "sgs_ex is not a table";
                break;
            }

            // CardFaces
            type = lua_getfield(l, -1, "CardFaces"); // { CardFaces, sgs_ex, sgs_registry }

            if (type != LUA_TTABLE) {
                qDebug() << "sgs_ex.CardFaces is not a table";
            } else {
                lua_pushnil(l); // { k(nil), CardFaces, sgs_ex, sgs_registry }
                while ((bool)(lua_next(l, -2))) { // { v, k, CardFaces, sgs_ex, sgs_registry }
                    // https://www.lua.org/manual/5.4/manual.html#lua_next
                    // do not use lua_tolstring (which lua_tostring uses it) on keys as Lua recommands
                    // so we use value["name"] instead
                    QString name;
                    type = lua_getfield(l, -1, "name"); // { name, v, k, CardFaces, sgs_ex, sgs_registry }
                    do {
                        if (type != LUA_TSTRING) {
                            qDebug() << "sgs.CardFaces contain an item which does not have a string \"name\", ignoring.";
                            break;
                        }

                        name = QString::fromUtf8(lua_tostring(l, -1));
                    } while (false);

                    lua_pop(l, 1); // { v, k, CardFaces, sgs_ex, sgs_registry }

                    // register CardFace in registry
                    if (name.isEmpty())
                        lua_pop(l, 1); // { k, CardFaces, sgs_ex, sgs_registry }
                    else {
                        int index = luaL_ref(l, -5); // {  k, CardFaces, sgs_ex, sgs_registry }
                        Q_ASSERT(index != LUA_NOREF);
                        reg.cardFaces.insert(name, index);
                    }
                } // { CardFaces, sgs_ex, sgs_registry }
            }

            reg.cardFacesTable = luaL_ref(l, -3); // { sgs_ex, sgs_registry }

            // Skills
            type = lua_getfield(l, -1, "Skills");
            if (type != LUA_TTABLE) {
                qDebug() << "sgs_ex.Skills is not a table";
            } else {
                // deal with Skills
                Q_UNIMPLEMENTED();
            }
            lua_pop(l, 1);

            // Packages
            type = lua_getfield(l, -1, "Packages");
            if (type != LUA_TTABLE) {
                qDebug() << "sgs_ex.Packages is not a table";
            } else {
                // deal with Packages
                Q_UNIMPLEMENTED();
            }
            lua_pop(l, 1);
        } while (false);

        lua_pop(l, 1); // { sgs_registry }
        lua_pop(l, 1); // { }
    }

    ~LuaStatePrivate()
    {
        lua_close(l);
    }
};
#endif

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

bool LuaState::pushCardFace(const QString &name)
{
    if (!d->reg.cardFaces.contains(name))
        return false;

    int r = d->reg.cardFaces.value(name, LUA_NOREF);
    if (r == LUA_NOREF)
        return false;

    // assuming all of following are successful since we dealt with it in previous code in CPP
    lua_getglobal(d->l, "sgs_registry"); // { sgs_registry }
    lua_rawgeti(d->l, -1, r); // { v, sgs_registry }
    lua_remove(d->l, -2); // { v }
    return true;
}

bool LuaState::pushCardFaces()
{
    if (d->reg.cardFacesTable == LUA_NOREF)
        return false;

    // assuming all of following are successful since we dealt with it in previous code in CPP
    lua_getglobal(d->l, "sgs_registry"); // { sgs_registry }
    lua_rawgeti(d->l, -1, d->reg.cardFacesTable); // { v, sgs_registry }
    lua_remove(d->l, -2); // { v }
    return true;
}

QStringList LuaState::cardFaceNames() const
{
    return d->reg.cardFaces.keys();
}

bool LuaState::pushSkill(const QString &name)
{
    // TODO
    return false;
}

bool LuaState::pushSkills()
{
    // TODO
    return false;
}

QStringList LuaState::skillNames() const
{
    // TODO
    return QStringList();
}

bool LuaState::pushPackage(const QString &name)
{
    // TODO
    return false;
}

bool LuaState::pushPackages()
{
    // TODO
    return false;
}

QStringList LuaState::packageNames() const
{
    // TODO
    return QStringList();
}

#ifndef Q_DOC
class LuaMultiThreadEnvironmentPrivate
{
public:
    QThreadStorage<LuaState *> stateStorage;
    QList<const Package *> packages;
    QList<const CardFace *> cardFaces;
    QList<const Skill *> skills;
};
#endif

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

#ifndef Q_DOC
namespace SgsEx {
CardFace *createNewLuaCardFace(const QString &name);
}
#endif

LuaMultiThreadEnvironment::LuaMultiThreadEnvironment()
    : d(new LuaMultiThreadEnvironmentPrivate)
{
    LuaState *firstLuaState = new LuaState;
    d->stateStorage.setLocalData(firstLuaState);

    // TODO: first LuaState is created, we should collect data from LuaState
    // notably Package, CardFace and Skill
    // Package maintains CardDescriptorList and GeneralList

    qDebug() << "CardFaces: " << firstLuaState->cardFaceNames();
    foreach (const QString &name, firstLuaState->cardFaceNames()) {
        CardFace *f = SgsEx::createNewLuaCardFace(name);
        if (f != nullptr)
            Sanguosha->registerCardFace(f);
        else
            qDebug() << "creation of cardFace " << name << "failed";
    }

    qDebug() << "Skills: " << firstLuaState->skillNames();
    foreach (const QString &name, firstLuaState->skillNames()) {
        // todo
        Q_UNUSED(name);
#if 0
        CardFace *f = SgsEx::createNewLuaCardFace(name);
        if (f != nullptr)
            Sanguosha->registerCardFace(f);
        else
            qDebug() << "creation of cardFace " << name << "failed";
#endif
    }

    qDebug() << "Packages: " << firstLuaState->packageNames();
    foreach (const QString &name, firstLuaState->packageNames()) {
        // todo
        Q_UNUSED(name);
#if 0
        CardFace *f = SgsEx::createNewLuaCardFace(name);
        if (f != nullptr)
            Sanguosha->registerCardFace(f);
        else
            qDebug() << "creation of cardFace " << name << "failed";
#endif
    }
}

LuaMultiThreadEnvironment *LuaMultiThreadEnvironment::self()
{
    static LuaMultiThreadEnvironment instance;
    return &instance;
}

namespace BuiltinExtension {

// Should this be here? Maybe Engine should be responsible for this instead
// Exit program when a pure-server program runs, and disable connecting to any server (except for localhost) on main window
// Does nothing when debug is on
void disableConnectToServer()
{
    Q_UNIMPLEMENTED();
}

} // namespace BuiltinExtension
