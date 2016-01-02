#include "generalselector.h"

#include <QApplication>
#include <QMessageBox>
#include "lua.hpp"
#include "util.h"

static GeneralSelector *Selector;



GeneralSelector *GeneralSelector::getInstance()
{
    if (Selector == NULL) {
        Selector = new GeneralSelector;
        //@todo: this setParent is illegitimate in QT and is equivalent to calling
        // setParent(NULL). So taking it off at the moment until we figure out
        // a way to do it.
        //Selector->setParent(Sanguosha);
        connect(qApp, &QApplication::aboutToQuit, Selector, &GeneralSelector::deleteLater);
    }

    return Selector;
}

GeneralSelector::GeneralSelector()
{
    L = CreateLuaState();
    int error = luaL_dofile(L, "lua/general_select.lua");
    if (error) {
        QString error_msg = lua_tostring(L, -1);
        QMessageBox::critical(NULL, QObject::tr("Lua script error"), error_msg);
        exit(1);
    } else
        initialize();
}


GeneralSelector::~GeneralSelector()
{
    lua_close(L);
}