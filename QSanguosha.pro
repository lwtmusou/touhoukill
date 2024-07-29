# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = QSanguosha
QT += network widgets
TEMPLATE = app
CONFIG += audio
win32: QT += winextras

!equals(QT_MAJOR_VERSION, "5") {
    error("QSanguosha requires Qt 5 after 5.6.")
}

lessThan(QT_MINOR_VERSION, 6) {
    error("QSanguosha requires Qt 5 after 5.6.")
}

CONFIG += c++11
CONFIG += lua

VERSION = 0.10.10
VERSIONNUMBER = 20240727

CONFIG += precompiled_header
PRECOMPILED_HEADER = src/pch.h

SOURCES += \
    src/client/aux-skills.cpp \
    src/client/client.cpp \
    src/client/clientplayer.cpp \
    src/client/clientstruct.cpp \
    src/core/card.cpp \
    src/core/engine.cpp \
    src/core/general.cpp \
    src/core/json.cpp \
    src/core/lua-wrapper.cpp \
    src/core/player.cpp \
    src/core/protocol.cpp \
    src/core/record-analysis.cpp \
    src/core/RoomState.cpp \
    src/core/settings.cpp \
    src/core/skill.cpp \
    src/core/structs.cpp \
    src/core/util.cpp \
    src/core/WrappedCard.cpp \
    src/dialog/AboutUs.cpp \
    src/dialog/cardoverview.cpp \
    src/dialog/choosegeneraldialog.cpp \
    src/dialog/configdialog.cpp \
    src/dialog/connectiondialog.cpp \
    src/dialog/distanceviewdialog.cpp \
    src/dialog/generaloverview.cpp \
    src/dialog/updatedialog.cpp \
    src/dialog/mainwindow.cpp \
    src/dialog/roleassigndialog.cpp \
    src/package/exppattern.cpp \
    src/package/maneuvering.cpp \
    src/package/package.cpp \
    src/package/protagonist.cpp \
    src/package/standard-cards.cpp \
    src/package/standard.cpp \
    src/package/th01-05.cpp \
    src/package/th06.cpp \
    src/package/th07.cpp \
    src/package/th08.cpp \
    src/package/th09.cpp \
    src/package/th10.cpp \
    src/package/th11.cpp \
    src/package/th12.cpp \
    src/package/th13.cpp \
    src/package/th14.cpp \
    src/package/th15.cpp \
    src/package/th16.cpp \
    src/package/th17.cpp \
    src/package/th18.cpp \
    src/package/th19.cpp \
    src/package/th99.cpp \
    src/package/thndj.cpp \
    src/package/touhougod.cpp \
    src/package/hegemonyGeneral.cpp \
    src/package/washout.cpp \
    src/server/ai.cpp \
    src/server/gamerule.cpp \
    src/server/generalselector.cpp \
    src/server/room.cpp \
    src/server/roomthread.cpp \
    src/server/server.cpp \
    src/server/serverplayer.cpp \
    src/ui/bubblechatbox.cpp \
    src/ui/button.cpp \
    src/ui/cardcontainer.cpp \
    src/ui/carditem.cpp \
    src/ui/chatwidget.cpp \
    src/ui/clientlogbox.cpp \
    src/ui/dashboard.cpp \
    src/ui/GenericCardContainerUI.cpp \
    src/ui/graphicspixmaphoveritem.cpp \
    src/ui/heroskincontainer.cpp \
    src/ui/indicatoritem.cpp \
    src/ui/magatamasItem.cpp \
    src/ui/photo.cpp \
    src/ui/pixmapanimation.cpp \
    src/ui/choosegeneralbox.cpp \
    src/ui/qsanbutton.cpp \
    src/ui/QSanSelectableItem.cpp \
    src/ui/rolecombobox.cpp \
    src/ui/roomscene.cpp \
    src/ui/sanfreetypefont.cpp \
    src/ui/sanshadowtextfont.cpp \
    src/ui/sansimpletextfont.cpp \
    src/ui/sanuiutils.cpp \
    src/ui/SkinBank.cpp \
    src/ui/sgswindow.cpp \
    src/ui/skinitem.cpp \
    src/ui/sprite.cpp \
    src/ui/startscene.cpp \
    src/ui/TablePile.cpp \
    src/ui/TimedProgressBar.cpp \
    src/ui/uiUtils.cpp \
    src/util/detector.cpp \
    src/util/nativesocket.cpp \
    src/util/recorder.cpp \
    src/main.cpp \
    src/ui/choosetriggerorderbox.cpp \
    src/ui/graphicsbox.cpp \
    src/ui/lightboxanimation.cpp \
    src/ui/chooseoptionsbox.cpp \
    src/ui/playercardbox.cpp \
    src/package/hegemonyCard.cpp \
    src/package/playground.cpp \
    src/ui/hegemonyrolecombobox.cpp \
    src/package/peasants_vs_landlord.cpp

HEADERS += \
    src/client/aux-skills.h \
    src/client/client.h \
    src/client/clientplayer.h \
    src/client/clientstruct.h \
    src/core/audio.h \
    src/core/card.h \
    src/core/compiler-specific.h \
    src/core/engine.h \
    src/core/general.h \
    src/core/json.h \
    src/core/lua-wrapper.h \
    src/core/player.h \
    src/core/protocol.h \
    src/core/record-analysis.h \
    src/core/RoomState.h \
    src/core/settings.h \
    src/core/skill.h \
    src/core/structs.h \
    src/core/util.h \
    src/core/WrappedCard.h \
    src/dialog/AboutUs.h \
    src/dialog/cardoverview.h \
    src/dialog/choosegeneraldialog.h \
    src/dialog/configdialog.h \
    src/dialog/connectiondialog.h \
    src/dialog/distanceviewdialog.h \
    src/dialog/generaloverview.h \
    src/dialog/updatedialog.h \
    src/dialog/mainwindow.h \
    src/dialog/roleassigndialog.h \
    src/package/exppattern.h \
    src/package/maneuvering.h \
    src/package/package.h \
    src/package/protagonist.h \
    src/package/standard-equips.h \
    src/package/standard.h \
    src/package/th01-05.h \
    src/package/th06.h \
    src/package/th07.h \
    src/package/th08.h \
    src/package/th09.h \
    src/package/th10.h \
    src/package/th11.h \
    src/package/th12.h \
    src/package/th13.h \
    src/package/th14.h \
    src/package/th15.h \
    src/package/th16.h \
    src/package/th17.h \
    src/package/th18.h \
    src/package/th19.h \
    src/package/th99.h \
    src/package/thndj.h \
    src/package/touhougod.h \
    src/package/hegemonyGeneral.h \
    src/package/washout.h \
    src/server/ai.h \
    src/server/gamerule.h \
    src/server/generalselector.h \
    src/server/room.h \
    src/server/roomthread.h \
    src/server/server.h \
    src/server/serverplayer.h \
    src/ui/bubblechatbox.h \
    src/ui/button.h \
    src/ui/cardcontainer.h \
    src/ui/carditem.h \
    src/ui/chatwidget.h \
    src/ui/clientlogbox.h \
    src/ui/dashboard.h \
    src/ui/GenericCardContainerUI.h \
    src/ui/graphicspixmaphoveritem.h \
    src/ui/choosegeneralbox.h \
    src/ui/heroskincontainer.h \
    src/ui/indicatoritem.h \
    src/ui/magatamasItem.h \
    src/ui/photo.h \
    src/ui/pixmapanimation.h \
    src/ui/qsanbutton.h \
    src/ui/QSanSelectableItem.h \
    src/ui/rolecombobox.h \
    src/ui/roomscene.h \
    src/ui/sanfreetypefont.h \
    src/ui/sanshadowtextfont.h \
    src/ui/sansimpletextfont.h \
    src/ui/sanuiutils.h \
    src/ui/SkinBank.h \
    src/ui/sgswindow.h \
    src/ui/skinitem.h \
    src/ui/sprite.h \
    src/ui/startscene.h \
    src/ui/TablePile.h \
    src/ui/TimedProgressBar.h \
    src/ui/uiUtils.h \
    src/util/detector.h \
    src/util/nativesocket.h \
    src/util/recorder.h \
    src/util/socket.h \
    src/ui/choosetriggerorderbox.h \
    src/ui/graphicsbox.h \
    src/ui/lightboxanimation.h \
    src/ui/hegemonyrolecombobox.h \
    src/ui/chooseoptionsbox.h \
    src/ui/playercardbox.h \
    src/package/hegemonyCard.h \
    src/package/playground.h \
    src/pch.h \
    src/package/peasants_vs_landlord.h

FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/generaloverview.ui \
    src/dialog/mainwindow.ui

INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/ui
INCLUDEPATH += src/util

# For automatic update signature verification
SOURCES += src/p256/p256.c
HEADERS += src/p256/p256.h
INCLUDEPATH += src/p256

win32{
    CONFIG += skip_target_version_ext
    RC_ICONS += resource/icon/sgs.ico
    QMAKE_TARGET_DESCRIPTION = "TouhouSatsu Main Program"
}

macx{
    ICON = resource/icon/sgs.icns
}

DEFINES += "QSGS_VERSION=\\\"$$VERSION\\\""
DEFINES += "QSGS_VERSIONNUMBER=\\\"$$VERSIONNUMBER\\\""

LIBS += -L.
win32-msvc*{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    !contains(QMAKE_HOST.arch, x86_64) {
        DEFINES += WIN32
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x86"
        QMAKE_LFLAGS += "/LARGEADDRESSAWARE"
    } else {
        DEFINES += WIN64
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x64"
    }
}
win32-g++{
    DEFINES += WIN32
    LIBS += -L"$$_PRO_FILE_PWD_/lib/win/MinGW"
    DEFINES += GPP
}
winrt{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += WINRT
    !winphone {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/winrt/x64"
    } else {
        DEFINES += WINPHONE
        contains($$QMAKESPEC, arm): LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/arm"
        else : LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/x86"
    }
}
macx{
    DEFINES += MAC
    LIBS += -L"$$_PRO_FILE_PWD_/lib/mac/lib"
    DEFINES += LUA_USE_MACOSX
}
ios{
    DEFINES += IOS
    CONFIG(iphonesimulator){
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/simulator/lib"
    }
    else {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/device/lib"
    }
}
linux{
    android{
        DEFINES += ANDROID
        ANDROID_LIBPATH = $$_PRO_FILE_PWD_/lib/android/$$ANDROID_ARCHITECTURE/lib
        LIBS += -L"$$ANDROID_LIBPATH"
    }
    else {
        DEFINES += LINUX
        !contains(QMAKE_HOST.arch, x86_64) {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x86"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x86
        }
        else {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x64"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x64
        }
        DEFINES += LUA_USE_LINUX
        LIBS += -ldl -lreadline
    }
}

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/fmod
    CONFIG(debug, debug|release): LIBS += -lfmodexL
    else:LIBS += -lfmodex
    SOURCES += src/core/audio.cpp

    android{
        CONFIG(debug, debug|release):ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodexL.so
        else:ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodex.so
    }
}

CONFIG(lua){

android:DEFINES += "\"getlocaledecpoint()='.'\""

    SOURCES += \
        src/lua/lzio.c \
        src/lua/lvm.c \
        src/lua/lundump.c \
        src/lua/ltm.c \
        src/lua/ltablib.c \
        src/lua/ltable.c \
        src/lua/lstrlib.c \
        src/lua/lstring.c \
        src/lua/lstate.c \
        src/lua/lparser.c \
        src/lua/loslib.c \
        src/lua/lopcodes.c \
        src/lua/lobject.c \
        src/lua/loadlib.c \
        src/lua/lmem.c \
        src/lua/lmathlib.c \
        src/lua/llex.c \
        src/lua/liolib.c \
        src/lua/linit.c \
        src/lua/lgc.c \
        src/lua/lfunc.c \
        src/lua/ldump.c \
        src/lua/ldo.c \
        src/lua/ldebug.c \
        src/lua/ldblib.c \
        src/lua/lctype.c \
        src/lua/lcorolib.c \
        src/lua/lcode.c \
        src/lua/lbitlib.c \
        src/lua/lbaselib.c \
        src/lua/lauxlib.c \
        src/lua/lapi.c
    HEADERS += \
        src/lua/lzio.h \
        src/lua/lvm.h \
        src/lua/lundump.h \
        src/lua/lualib.h \
        src/lua/luaconf.h \
        src/lua/lua.hpp \
        src/lua/lua.h \
        src/lua/ltm.h \
        src/lua/ltable.h \
        src/lua/lstring.h \
        src/lua/lstate.h \
        src/lua/lparser.h \
        src/lua/lopcodes.h \
        src/lua/lobject.h \
        src/lua/lmem.h \
        src/lua/llimits.h \
        src/lua/llex.h \
        src/lua/lgc.h \
        src/lua/lfunc.h \
        src/lua/ldo.h \
        src/lua/ldebug.h \
        src/lua/lctype.h \
        src/lua/lcode.h \
        src/lua/lauxlib.h \
        src/lua/lapi.h
    INCLUDEPATH += src/lua
}

SWIGFILES += $$_PRO_FILE_PWD_/swig/sanguosha.i
SWIGDEPENDS += $$_PRO_FILE_PWD_/swig/sanguosha.i \
               $$_PRO_FILE_PWD_/swig/ai.i \
               $$_PRO_FILE_PWD_/swig/card.i \
               $$_PRO_FILE_PWD_/swig/general_select.i \
               $$_PRO_FILE_PWD_/swig/list.i \
               $$_PRO_FILE_PWD_/swig/luaskills.i \
               $$_PRO_FILE_PWD_/swig/native.i \
               $$_PRO_FILE_PWD_/swig/naturalvar.i \
               $$_PRO_FILE_PWD_/swig/qvariant.i

SWIG_bin = "swig"
contains(QMAKE_HOST.os, "Windows"): SWIG_bin = "$$_PRO_FILE_PWD_/tools/swig/swig.exe"

swig.commands = "$$system_path($$SWIG_bin) -c++ -lua -cppext cpp -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}"
swig.CONFIG = target_predeps
swig.dependency_type = TYPE_C
swig.depends = $$SWIGDEPENDS
swig.input = SWIGFILES
swig.name = "Generating ${QMAKE_FILE_NAME}..."
swig.output = ${QMAKE_FILE_BASE}_wrap.cpp
swig.variable_out = SOURCES

QMAKE_EXTRA_COMPILERS += swig

!build_pass{
    system("$$dirname(QMAKE_QMAKE)/lrelease $$_PRO_FILE_PWD_/builds/sanguosha.ts -qm $$_PRO_FILE_PWD_/sanguosha.qm")
}

TRANSLATIONS += builds/sanguosha.ts

CONFIG(debug, debug|release): LIBS += -lfreetype_D
else:LIBS += -lfreetype

INCLUDEPATH += $$_PRO_FILE_PWD_/include/freetype
DEPENDPATH += $$_PRO_FILE_PWD_/include/freetype

#ANDROID_PACKAGE_SOURCE_DIR = $$_PRO_FILE_PWD_/resource/android

