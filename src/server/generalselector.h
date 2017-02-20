#ifndef _GENERAL_SELECTOR_H
#define _GENERAL_SELECTOR_H

#include "lua-wrapper.h"

class ServerPlayer;
struct lua_State;
class Room;

class GeneralSelector : public QObject
{
    Q_OBJECT

public:
    GeneralSelector(Room *room);

    QString selectFirst(ServerPlayer *player, const QStringList &candidates);
    QString selectSecond(ServerPlayer *player, const QStringList &candidates);
    QString select3v3(ServerPlayer *player, const QStringList &candidates);
    QString select1v1(const QStringList &candidates);
    QStringList arrange3v3(ServerPlayer *player);
    QStringList arrange1v1(ServerPlayer *player);

private:
    void initialize();
    void callLuaInitialize();
    lua_State *L;

    LuaFunction initializeFunc;
    LuaFunction selectFirstFunc;
    LuaFunction selectSecondFunc;
    LuaFunction select3v3Func;
    LuaFunction select1v1Func;
    LuaFunction arrange3v3Func;
    LuaFunction arrange1v1Func;
};

#endif
