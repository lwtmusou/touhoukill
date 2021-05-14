#ifndef _LUA_WRAPPER_H
#define _LUA_WRAPPER_H

#include <QMetaObject>
#include <QObject>

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
    const lua_State *data() const;

private:
    LuaStatePrivate *d;
    friend class LuaMultiThreadEnvironment;

    LuaState();
    Q_DISABLE_COPY_MOVE(LuaState)
};

class LuaMultiThreadEnvironment
{
public:
    static LuaState *luaStatePerThread();

    static const QList<const Package *> &packages();
    static const QList<const CardFace *> &cardFaces();
    static const QList<const Skill *> &skills();

private:
    LuaMultiThreadedEnvironmentPrivate *d;
    Q_DISABLE_COPY_MOVE(LuaMultiThreadEnvironment)

    LuaMultiThreadEnvironment();
    static LuaMultiThreadEnvironment *self();
};

#endif
