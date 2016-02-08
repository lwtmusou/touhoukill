#include "generalselector.h"

#include <QApplication>
#include <QMessageBox>
#include "lua.hpp"
#include "room.h"

GeneralSelector::GeneralSelector(Room *room)
    : QObject(room)
{
    L = room->getLuaState();
    int error = luaL_dofile(L, "lua/general_select.lua");
    if (error) {
        QString error_msg = lua_tostring(L, -1);
        QMessageBox::critical(NULL, QObject::tr("Lua script error"), error_msg);
        exit(1);
    } else
        initialize();
}

