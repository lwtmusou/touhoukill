#ifndef _LUA_WRAPPER_H
#define _LUA_WRAPPER_H

#include <QObject>

class LuaStatePrivate;

class LuaState : public QObject
{
    Q_OBJECT

public:
    explicit LuaState(QObject *parent = nullptr);
    ~LuaState() override;

private:
    LuaStatePrivate *d;
};

#endif
