#ifndef TOUHOUKILL_LUA_WRAPPER_H
#define TOUHOUKILL_LUA_WRAPPER_H

#include "qsgscore.h"
#include <QObject>
#include <QPointer>

class LuaStatePrivate;
class LuaMultiThreadEnvironment;
class LuaMultiThreadEnvironmentPrivate;
class Package;
class CardFace;
class Skill;
struct lua_State;

class QSGS_CORE_EXPORT LuaState final : public QObject
{
    Q_OBJECT

public:
    ~LuaState() override;

    lua_State *data();

    // Helper functions
    bool pushCardFace(const QString &name);
    bool pushCardFaces();
    QStringList cardFaceNames() const;
    bool pushSkill(const QString &name);
    bool pushSkills();
    QStringList skillNames() const;
    bool pushTrigger(const QString &name);
    bool pushTriggers();
    QStringList triggerNames() const;
    bool pushPackage(const QString &name);
    bool pushPackages();
    QStringList packageNames() const;

private:
    LuaStatePrivate *d;
    friend class LuaMultiThreadEnvironment;

    LuaState();
    Q_DISABLE_COPY_MOVE(LuaState)
};

class QSGS_CORE_EXPORT LuaStatePointer final
{
public:
    LuaStatePointer(const LuaStatePointer &) = default;
    LuaStatePointer &operator=(const LuaStatePointer &) = default;
    inline LuaStatePointer(LuaState *p)
        : d(p)
    {
    }

    inline LuaState *data() const
    {
        return d.data();
    }

    inline LuaState *operator->() const
    {
        return d.data();
    }

    inline LuaState &operator*() const
    {
        Q_ASSERT(!d.isNull());
        return *(d.data());
    }

    inline operator LuaState *() const
    {
        return d.data();
    }

    inline bool isNull() const
    {
        return d.isNull();
    }

    inline operator lua_State *() const
    {
        if (d.isNull())
            return nullptr;

        return d->data();
    }

private:
    LuaStatePointer() = delete;
    QPointer<LuaState> d;
};

class QSGS_CORE_EXPORT LuaMultiThreadEnvironment final
{
public:
    static LuaStatePointer luaStateForCurrentThread();

    static const QList<const Package *> &packages();
    static const QList<const CardFace *> &cardFaces();
    static const QList<const Skill *> &skills();

    static const QString &luaVersion();
    static const QString &luaCopyright();

private:
    LuaMultiThreadEnvironmentPrivate *d;
    Q_DISABLE_COPY_MOVE(LuaMultiThreadEnvironment)

    LuaMultiThreadEnvironment();
    static LuaMultiThreadEnvironment *self();
};

namespace BuiltinExtension {

// Should this be here? Maybe Engine should be responsible for this instead
// Exit program when a pure-server program runs, and disable connecting to any server (except for localhost) on main window
// Does nothing when debug is on
QSGS_CORE_EXPORT void disableConnectToServer();

} // namespace BuiltinExtension
#endif
