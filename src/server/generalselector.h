#ifndef _GENERAL_SELECTOR_H
#define _GENERAL_SELECTOR_H

#include "lua-wrapper.h"

class ServerPlayer;
struct lua_State;


// singleton class
class GeneralSelector : public QObject
{
    Q_OBJECT

public:
    static GeneralSelector *getInstance();
    ~GeneralSelector();

    QString selectFirst(ServerPlayer *player, const QStringList &candidates);
    QString selectSecond(ServerPlayer *player, const QStringList &candidates);
    QString select3v3(ServerPlayer *player, const QStringList &candidates);
    QString select1v1(const QStringList &candidates);
    QStringList arrange3v3(ServerPlayer *player);
    QStringList arrange1v1(ServerPlayer *player);
    int get1v1ArrangeValue(const QString &name);

private:
    GeneralSelector();
    void initialize();
    void callLuaInitialize();
    void pushSelf();

    lua_State *L;

    LuaFunction initializeFunc;
    LuaFunction selectFirstFunc;
    LuaFunction selectSecondFunc;
    LuaFunction select3v3Func;
    LuaFunction select1v1Func;
    LuaFunction arrange3v3Func;
    LuaFunction arrange1v1Func;
    LuaFunction get1v1ArrangeValueFunc;

};

#endif

