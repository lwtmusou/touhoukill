#ifndef _LUA_WRAPPER_H
#define _LUA_WRAPPER_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>

class LuaStatePrivate;
class LuaMultiThreadEnvironment;
class LuaMultiThreadedEnvironmentPrivate;
class Package;
class CardFace;
class Skill;
struct lua_State;

class LuaState : public QObject
{
    Q_OBJECT

public:
    typedef int LuaFunction;
    ~LuaState() override;

    lua_State *data();

private:
    LuaStatePrivate *d;
    friend class LuaMultiThreadEnvironment;

    LuaState();
    Q_DISABLE_COPY_MOVE(LuaState)
};

// A wrapper for "LuaState *const" which supports implicit type conversion to lua_State *
class LuaStatePointer
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

class LuaMultiThreadEnvironment
{
public:
    static LuaStatePointer luaStateForCurrentThread();

    static const QList<const Package *> &packages();
    static const QList<const CardFace *> &cardFaces();
    static const QList<const Skill *> &skills();

    static const QString &luaVersion();
    static const QString &luaCopyright();

private:
    LuaMultiThreadedEnvironmentPrivate *d;
    Q_DISABLE_COPY_MOVE(LuaMultiThreadEnvironment)

    LuaMultiThreadEnvironment();
    static LuaMultiThreadEnvironment *self();
};

#endif
