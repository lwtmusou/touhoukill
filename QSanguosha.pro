# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------

## Project basics
TARGET = QSanguosha
TEMPLATE = app
# Do NOT add `5compat` -- Qt5Compat is globally forbidden in this project
# (see qml/plan.md section 0). Use Qt 6 native alternatives
# (e.g. MultiEffect via `import QtQuick.Effects`).
QT += network widgets quick quickwidgets

VERSION = 0.10.11
VERSIONNUMBER = 20250705

## Compiler / build flags
CONFIG += c++17
CONFIG += precompiled_header
PRECOMPILED_HEADER = src/pch.h
win32:CONFIG(debug, debug|release): CONFIG += console

## Feature toggles
!macx: CONFIG += audio
CONFIG += lua

## Source files (C++)
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
    src/dialog/gameoverdialog.cpp \
    src/dialog/generaloverview.cpp \
    src/dialog/mainwindow.cpp \
    src/dialog/roleassigndialog.cpp \
    src/dialog/serverdialog.cpp \
    src/dialog/TimedProgressBar.cpp \
    src/dialog/updatedialog.cpp \
    src/dialog/uilegacy/SkinBank.cpp \
    src/main.cpp \
    src/package/exppattern.cpp \
    src/package/hegemonyCard.cpp \
    src/package/hegemonyGeneral.cpp \
    src/package/maneuvering.cpp \
    src/package/package.cpp \
    src/package/peasants_vs_landlord.cpp \
    src/package/playground.cpp \
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
    src/package/th20.cpp \
    src/package/th99.cpp \
    src/package/thndj.cpp \
    src/package/touhougod.cpp \
    src/package/washout.cpp \
    src/qmlui/qmlui.cpp \
    src/qmlui/roomscene.cpp \
    src/server/ai.cpp \
    src/server/gamerule.cpp \
    src/server/generalselector.cpp \
    src/server/room.cpp \
    src/server/roomthread.cpp \
    src/server/server.cpp \
    src/server/serverplayer.cpp \
    src/util/detector.cpp \
    src/util/nativesocket.cpp \
    src/util/recorder.cpp

## Header files (C++)
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
    src/dialog/gameoverdialog.h \
    src/dialog/generaloverview.h \
    src/dialog/mainwindow.h \
    src/dialog/roleassigndialog.h \
    src/dialog/serverdialog.h \
    src/dialog/TimedProgressBar.h \
    src/dialog/updatedialog.h \
    src/dialog/uilegacy/SkinBank.h \
    src/dialog/uilegacy/qsanbutton.h \
    src/package/exppattern.h \
    src/package/hegemonyCard.h \
    src/package/hegemonyGeneral.h \
    src/package/maneuvering.h \
    src/package/package.h \
    src/package/peasants_vs_landlord.h \
    src/package/playground.h \
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
    src/package/th20.h \
    src/package/th99.h \
    src/package/thndj.h \
    src/package/touhougod.h \
    src/package/washout.h \
    src/pch.h \
    src/qmlui/qmlui.h \
    src/qmlui/roomscene.h \
    src/server/ai.h \
    src/server/gamerule.h \
    src/server/generalselector.h \
    src/server/room.h \
    src/server/roomthread.h \
    src/server/server.h \
    src/server/serverplayer.h \
    src/util/detector.h \
    src/util/nativesocket.h \
    src/util/recorder.h \
    src/util/socket.h

## Qt Designer forms (.ui)
FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/generaloverview.ui \
    src/dialog/mainwindow.ui

## Include paths
INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/qmlui
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/util

## QML files (lupdate-only)
# Listed under SOURCES (not OTHER_FILES) so that:
# (1) lupdate-pro scans them for qsTr() -- OTHER_FILES is not scanned;
# (2) Qt Creator shows them in the project tree under Sources.
# Wrapped in lupdate_only so qmake build ignores this block entirely
# (.qml/.js have no compiler rule, so they never enter the build anyway).
lupdate_only {
SOURCES += \
    qml/CardContainer.qml \
    qml/CardItem.qml \
    qml/ChooseGeneralBox.qml \
    qml/Dashboard.qml \
    qml/EquipAreaBase.qml \
    qml/EquipSlot.qml \
    qml/GraphicsBox.qml \
    qml/HandcardNum.qml \
    qml/HegRoleComboBox.qml \
    qml/JudgeArea.qml \
    qml/KingdomImage.qml \
    qml/Magatama.qml \
    qml/Magatamas.qml \
    qml/main.qml \
    qml/PhaseItem.qml \
    qml/Photo.qml \
    qml/PhotoEquipArea.qml \
    qml/PrivatePileArea.qml \
    qml/PromptBox.qml \
    qml/QSanButton.qml \
    qml/QSanSkillButton.qml \
    qml/RoleComboBox.qml \
    qml/RoomScene.qml \
    qml/RootItem.qml \
    qml/SeatNumberItem.qml \
    qml/SelfEquipArea.qml \
    qml/SkillDock.qml \
    qml/StartScene.qml \
    qml/TablePile.qml \
    qml/VerticalText.qml
}

## Version defines
DEFINES += "QSGS_VERSION=\\\"$$VERSION\\\""
DEFINES += "QSGS_VERSIONNUMBER=\\\"$$VERSIONNUMBER\\\""

## Platform-specific settings
win32 {
    CONFIG += skip_target_version_ext
    RC_ICONS += resource/icon/sgs.ico
    QMAKE_TARGET_DESCRIPTION = "TouhouSatsu Main Program"
}
macx {
    ICON = resource/icon/sgs.icns
}

## Platform-specific libs and defines
LIBS += -L.
win32-msvc* {
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
win32-g++ {
    DEFINES += WIN32
    LIBS += -L"$$_PRO_FILE_PWD_/lib/win/MinGW"
    DEFINES += GPP
}
winrt {
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += WINRT
    !winphone {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/winrt/x64"
    } else {
        DEFINES += WINPHONE
        contains($$QMAKESPEC, arm): LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/arm"
        else: LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/x86"
    }
}
macx {
    DEFINES += MAC
    DEFINES += LUA_USE_MACOSX
}
ios {
    DEFINES += IOS
    CONFIG(iphonesimulator) {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/simulator/lib"
    } else {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/device/lib"
    }
}
linux {
    android {
        DEFINES += ANDROID
        ANDROID_LIBPATH = $$_PRO_FILE_PWD_/lib/android/$$ANDROID_ARCHITECTURE/lib
        LIBS += -L"$$ANDROID_LIBPATH"
    } else {
        DEFINES += LINUX
        !contains(QMAKE_HOST.arch, x86_64) {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x86"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x86
        } else {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x64"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x64
        }
        DEFINES += LUA_USE_LINUX
        LIBS += -ldl -lreadline
    }
}

## Optional feature: audio (fmod)
CONFIG(audio) {
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/fmod
    SOURCES += src/core/audio.cpp
    CONFIG(debug, debug|release): LIBS += -lfmodexL
    else: LIBS += -lfmodex
    android {
        CONFIG(debug, debug|release): ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodexL.so
        else: ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodex.so
    }
}

## Optional feature: Lua (embedded interpreter + SWIG bindings)
CONFIG(lua) {
    android: DEFINES += "\"getlocaledecpoint()='.'\""
    SOURCES += \
        src/lua/lapi.c \
        src/lua/lauxlib.c \
        src/lua/lbaselib.c \
        src/lua/lbitlib.c \
        src/lua/lcode.c \
        src/lua/lcorolib.c \
        src/lua/lctype.c \
        src/lua/ldblib.c \
        src/lua/ldebug.c \
        src/lua/ldo.c \
        src/lua/ldump.c \
        src/lua/lfunc.c \
        src/lua/lgc.c \
        src/lua/linit.c \
        src/lua/liolib.c \
        src/lua/llex.c \
        src/lua/lmathlib.c \
        src/lua/lmem.c \
        src/lua/loadlib.c \
        src/lua/lobject.c \
        src/lua/lopcodes.c \
        src/lua/loslib.c \
        src/lua/lparser.c \
        src/lua/lstate.c \
        src/lua/lstring.c \
        src/lua/lstrlib.c \
        src/lua/ltable.c \
        src/lua/ltablib.c \
        src/lua/ltm.c \
        src/lua/lundump.c \
        src/lua/lvm.c \
        src/lua/lzio.c
    HEADERS += \
        src/lua/lapi.h \
        src/lua/lauxlib.h \
        src/lua/lcode.h \
        src/lua/lctype.h \
        src/lua/ldebug.h \
        src/lua/ldo.h \
        src/lua/lfunc.h \
        src/lua/lgc.h \
        src/lua/llimits.h \
        src/lua/lmem.h \
        src/lua/lobject.h \
        src/lua/lopcodes.h \
        src/lua/lparser.h \
        src/lua/lstate.h \
        src/lua/lstring.h \
        src/lua/ltable.h \
        src/lua/ltm.h \
        src/lua/lua.h \
        src/lua/lua.hpp \
        src/lua/luaconf.h \
        src/lua/lualib.h \
        src/lua/lundump.h \
        src/lua/lvm.h \
        src/lua/lzio.h
    INCLUDEPATH += src/lua
}

## SWIG: generate Lua bindings from .i files
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
contains(QMAKE_HOST.os, "Darwin"): contains(QMAKE_HOST.arch, "arm64"): SWIG_bin = "/opt/homebrew/bin/swig"

swig.commands = "$$system_path($$SWIG_bin) -c++ -lua -cppext cpp -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}"
swig.CONFIG = target_predeps
swig.dependency_type = TYPE_C
swig.depends = $$SWIGDEPENDS
swig.input = SWIGFILES
swig.name = "Generating ${QMAKE_FILE_NAME}..."
swig.output = ${QMAKE_FILE_BASE}_wrap.cpp
swig.variable_out = SOURCES

QMAKE_EXTRA_COMPILERS += swig

## Translations
TRANSLATIONS += builds/sanguosha.ts
!build_pass {
    system("$$dirname(QMAKE_QMAKE)/lrelease $$_PRO_FILE_PWD_/builds/sanguosha.ts -qm $$_PRO_FILE_PWD_/sanguosha.qm")
}

# ANDROID_PACKAGE_SOURCE_DIR = $$_PRO_FILE_PWD_/resource/android
