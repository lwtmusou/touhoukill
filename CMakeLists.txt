# CMake 3.20 is needed for USE_SWIG_DEPENDENCIES
cmake_minimum_required(VERSION 3.20)

if ( NOT ( ( CMAKE_GENERATOR MATCHES "Makefiles" ) OR ( CMAKE_GENERATOR MATCHES "Ninja" ) ) )
    message(FATAL_ERROR "We only support makefile and ninja generator.")
endif()

project(QSanguosha
    VERSION 0.10.3
    )

set(VERSION_NUMBER_QSanguosha 20210504)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

find_package(Qt5 COMPONENTS Widgets Network LinguistTools Multimedia  REQUIRED) # Locate the position of Qt5.

if (WIN32)
    find_package(Qt5 COMPONENTS WinExtras REQUIRED)
elseif(APPLE)
    add_compile_definitions(LUA_USE_MACOSX)
else()
    add_compile_definitions(LUA_USE_LINUX)
endif()

find_program(SWIG_EXECUTABLE swig)
if (WIN32)
    find_program(SWIG_EXECUTABLE swig.exe ${CMAKE_CURRENT_LIST_DIR}/tools/swig)
endif()

set(SWIG_USE_SWIG_DEPENDENCIES true)
set(CMAKE_SWIG_FLAGS "-nodefaultctor" "-nodefaultdtor")

find_package(SWIG COMPONENTS lua REQUIRED)
include(UseSWIG)

include_directories(
    "src/audio"
    "src/client"
    "src/core"
    "src/dialog"
    "src/package"
    "src/server"
    "src/ui"
    "src/util"
    "src"
)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(oggvorbisfile IMPORTED_TARGET GLOBAL ogg>=1.0.0 vorbis>=1.0.0 vorbisfile>=1.0.0)

    # Should we use "lua>=5.4 to indicate a lua 5.4? Lua is not even source compatible between minor versions!"
    pkg_search_module(lua54 IMPORTED_TARGET GLOBAL lua-5.4 lua5.4 lua54)
endif()

set_property(SOURCE src/swig/sgs.i
    PROPERTY
    CPLUSPLUS ON
    )
swig_add_library(sanguosha_wrap
    TYPE STATIC
    LANGUAGE lua
    SOURCES src/swig/sgs.i
    )

# ONLY FOR THEIR INCLUDE DIRECTORIES! Compile fails if we don't add the link libraries.
# Seems wierd since we don't even link the libraries here
target_link_libraries(sanguosha_wrap Qt5::Core)
if (lua54_FOUND)
    target_link_libraries(sanguosha_wrap PkgConfig::lua54)
endif()

set (QSanguosha_PKG_SRCS
    src/package/hegemonyCard.cpp
    src/package/hegemonyGeneral.cpp
    src/package/maneuvering.cpp
    src/package/playground.cpp
    src/package/protagonist.cpp
    src/package/standard-cards.cpp
    src/package/standard.cpp
    src/package/testCard.cpp
    src/package/th01-05.cpp
    src/package/th06.cpp
    src/package/th07.cpp
    src/package/th08.cpp
    src/package/th09.cpp
    src/package/th10.cpp
    src/package/th11.cpp
    src/package/th12.cpp
    src/package/th13.cpp
    src/package/th14.cpp
    src/package/th15.cpp
    src/package/th16.cpp
    src/package/th17.cpp
    src/package/th99.cpp
    src/package/thndj.cpp
    src/package/touhougod.cpp
)

set (QSanguosha_SRCS

    src/audio/audio.cpp

    src/client/aux-skills.cpp
    src/client/client.cpp
    src/client/clientplayer.cpp
    src/client/clientstruct.cpp
    src/core/card.cpp
    src/core/CardFace.cpp
    src/core/engine.cpp
    src/core/game-logic.cpp
    src/core/game-state.cpp
    src/core/general.cpp
    src/core/json.cpp
    src/core/lua-wrapper.cpp
    src/core/package.cpp
    src/core/player.cpp
    src/core/protocol.cpp
    src/core/RoomObject.cpp
    src/core/settings.cpp
    src/core/skill.cpp
    src/core/structs.cpp
    src/core/util.cpp
    src/dialog/AboutUs.cpp
    src/dialog/cardoverview.cpp
    src/dialog/choosegeneraldialog.cpp
    src/dialog/configdialog.cpp
    src/dialog/connectiondialog.cpp
    src/dialog/distanceviewdialog.cpp
    src/dialog/generaloverview.cpp
    src/dialog/mainwindow.cpp
    src/dialog/roleassigndialog.cpp
    src/dialog/updatedialog.cpp
    src/main.cpp
    src/package/exppattern.cpp
    src/server/gamerule.cpp
    src/server/generalselector.cpp
    src/server/room.cpp
    src/server/roomthread.cpp
    src/server/server.cpp
    src/server/serverplayer.cpp
    src/ui/bubblechatbox.cpp
    src/ui/button.cpp
    src/ui/cardcontainer.cpp
    src/ui/carditem.cpp
    src/ui/chatwidget.cpp
    src/ui/choosegeneralbox.cpp
    src/ui/chooseoptionsbox.cpp
    src/ui/choosetriggerorderbox.cpp
    src/ui/clientlogbox.cpp
    src/ui/dashboard.cpp
    src/ui/GenericCardContainerUI.cpp
    src/ui/graphicsbox.cpp
    src/ui/graphicspixmaphoveritem.cpp
    src/ui/hegemonyrolecombobox.cpp
    src/ui/heroskincontainer.cpp
    src/ui/indicatoritem.cpp
    src/ui/lightboxanimation.cpp
    src/ui/magatamasItem.cpp
    src/ui/photo.cpp
    src/ui/pixmapanimation.cpp
    src/ui/playercardbox.cpp
    src/ui/qsanbutton.cpp
    src/ui/QSanSelectableItem.cpp
    src/ui/rolecombobox.cpp
    src/ui/roomscene.cpp
    src/ui/sgswindow.cpp
    src/ui/SkinBank.cpp
    src/ui/skinitem.cpp
    src/ui/sprite.cpp
    src/ui/startscene.cpp
    src/ui/TablePile.cpp
    src/ui/TimedProgressBar.cpp
    src/ui/uiUtils.cpp
    src/util/detector.cpp
    src/util/nativesocket.cpp
    src/util/recorder.cpp

    #${QSanguosha_PKG_SRCS}
)

set (QSanguosha_UIS
    src/dialog/cardoverview.ui
    src/dialog/configdialog.ui
    src/dialog/generaloverview.ui
    src/dialog/mainwindow.ui
)


set(TS_FILES builds/sanguosha.ts)
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "ts")
qt5_add_translation(QM_FILES ${TS_FILES})

set (QSanguosha_PKG_MOCS
    src/package/hegemonyCard.h
    src/package/hegemonyGeneral.h
    src/package/maneuvering.h
    src/package/playground.h
    src/package/protagonist.h
    src/package/standard-equips.h
    src/package/standard.h
    src/package/testCard.h
    src/package/th01-05.h
    src/package/th06.h
    src/package/th07.h
    src/package/th08.h
    src/package/th09.h
    src/package/th10.h
    src/package/th11.h
    src/package/th12.h
    src/package/th13.h
    src/package/th14.h
    src/package/th15.h
    src/package/th16.h
    src/package/th17.h
    src/package/th99.h
    src/package/thndj.h
    src/package/touhougod.h

)

set (QSanguosha_MOCS
    src/audio/audio.h
    src/client/aux-skills.h
    src/client/client.h
    src/client/clientplayer.h
    src/client/clientstruct.h
    src/core/card.h
    src/core/CardFace.h
    src/core/engine.h
    src/core/game-logic.h
    src/core/game-state.h
    src/core/general.h
    src/core/json.h
    src/core/lua-wrapper.h
    src/core/package.h
    src/core/player.h
    src/core/protocol.h
    src/core/RoomObject.h
    src/core/settings.h
    src/core/skill.h
    src/core/structs.h
    src/core/util.h
    src/dialog/AboutUs.h
    src/dialog/cardoverview.h
    src/dialog/choosegeneraldialog.h
    src/dialog/configdialog.h
    src/dialog/connectiondialog.h
    src/dialog/distanceviewdialog.h
    src/dialog/generaloverview.h
    src/dialog/mainwindow.h
    src/dialog/roleassigndialog.h
    src/dialog/updatedialog.h
    src/package/exppattern.h
    src/pch.h
    src/server/gamerule.h
    src/server/generalselector.h
    src/server/room.h
    src/server/roomthread.h
    src/server/server.h
    src/server/serverplayer.h
    src/ui/bubblechatbox.h
    src/ui/button.h
    src/ui/cardcontainer.h
    src/ui/carditem.h
    src/ui/chatwidget.h
    src/ui/choosegeneralbox.h
    src/ui/chooseoptionsbox.h
    src/ui/choosetriggerorderbox.h
    src/ui/clientlogbox.h
    src/ui/dashboard.h
    src/ui/GenericCardContainerUI.h
    src/ui/graphicsbox.h
    src/ui/graphicspixmaphoveritem.h
    src/ui/hegemonyrolecombobox.h
    src/ui/heroskincontainer.h
    src/ui/indicatoritem.h
    src/ui/lightboxanimation.h
    src/ui/magatamasItem.h
    src/ui/photo.h
    src/ui/pixmapanimation.h
    src/ui/playercardbox.h
    src/ui/qsanbutton.h
    src/ui/QSanSelectableItem.h
    src/ui/rolecombobox.h
    src/ui/roomscene.h
    src/ui/sgswindow.h
    src/ui/SkinBank.h
    src/ui/skinitem.h
    src/ui/sprite.h
    src/ui/startscene.h
    src/ui/TablePile.h
    src/ui/TimedProgressBar.h
    src/ui/uiUtils.h
    src/util/detector.h
    src/util/nativesocket.h
    src/util/recorder.h
    src/util/socket.h

    #${QSanguosha_PKG_MOCS}
)
if (WIN32)
    set(QSanguosha_SRCS ${QSanguosha_SRCS} QSanguosha_resource.rc)
    set(QSanguosha_Icon_RC ${CMAKE_CURRENT_LIST_DIR}/resource/icon/sgs.ico)
endif()

add_executable (QSanguosha WIN32 MACOSX_BUNDLE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} lua/lua.qrc )

set_property(TARGET QSanguosha PROPERTY AUTOMOC ON)
set_property(TARGET QSanguosha PROPERTY AUTOUIC ON)
set_property(TARGET QSanguosha PROPERTY AUTORCC ON)

target_link_libraries (QSanguosha sanguosha_wrap Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Multimedia)
target_compile_definitions(QSanguosha PUBLIC AUDIO_SUPPORT QT_DISABLE_DEPRECATED_BEFORE=0x60000 VERSION=${QSanguosha_VERSION} VERSIONNUMBER=${VERSION_NUMBER_QSanguosha})

if (NOT oggvorbisfile_FOUND)
    set(OGG_FILE
        src/ogg/backends.h
        src/ogg/bitrate.h
        src/ogg/codebook.h
        src/ogg/codec_internal.h
        src/ogg/crctable.h
        src/ogg/envelope.h
        src/ogg/highlevel.h
        src/ogg/lookup.h
        src/ogg/lookup_data.h
        src/ogg/lpc.h
        src/ogg/lsp.h
        src/ogg/masking.h
        src/ogg/mdct.h
        src/ogg/misc.h
        src/ogg/os.h
        src/ogg/psy.h
        src/ogg/registry.h
        src/ogg/scales.h
        src/ogg/smallft.h
        src/ogg/window.h

        src/ogg/analysis.c
        src/ogg/bitrate.c
        src/ogg/bitwise.c
        src/ogg/block.c
        src/ogg/codebook.c
        src/ogg/envelope.c
        src/ogg/floor0.c
        src/ogg/floor1.c
        src/ogg/framing.c
        src/ogg/info.c
        src/ogg/lookup.c
        src/ogg/lpc.c
        src/ogg/lsp.c
        src/ogg/mapping0.c
        src/ogg/mdct.c
        src/ogg/psy.c
        src/ogg/registry.c
        src/ogg/res0.c
        src/ogg/sharedbook.c
        src/ogg/smallft.c
        src/ogg/synthesis.c
        src/ogg/vorbisfile.c
        src/ogg/window.c

        include/oggvorbisfile/ogg/ogg.h
        include/oggvorbisfile/ogg/os_types.h
        include/oggvorbisfile/ogg/config_types.h
        include/oggvorbisfile/vorbis/codec.h
        include/oggvorbisfile/vorbis/vorbisfile.h
    )

    include_directories("include/oggvorbisfile")
    add_library(oggvorbisfile STATIC ${OGG_FILE})

    if (MSVC)
        target_compile_options(oggvorbisfile BEFORE PUBLIC "-wd4244" PUBLIC "-wd4305" PUBLIC "-wd4267")
    endif()
    target_link_libraries (QSanguosha oggvorbisfile)
else()
    # target_compile_options(QSanguosha oggvorbisfile_CFLAGS)
    target_link_libraries (QSanguosha PkgConfig::oggvorbisfile)
endif()

if (NOT lua54_FOUND)
    set(LUA_FILE
        src/lua/lfunc.c
        src/lua/lgc.c
        src/lua/linit.c
        src/lua/liolib.c
        src/lua/llex.c
        src/lua/lmathlib.c
        src/lua/lmem.c
        src/lua/loadlib.c
        src/lua/lobject.c
        src/lua/lopcodes.c
        src/lua/loslib.c
        src/lua/lparser.c
        src/lua/lstate.c
        src/lua/lstring.c
        src/lua/lstrlib.c
        src/lua/ltable.c
        src/lua/ltablib.c
        src/lua/ltm.c
        src/lua/lundump.c
        src/lua/lutf8lib.c
        src/lua/lvm.c
        src/lua/lzio.c
        src/lua/lapi.h
        src/lua/lcode.h
        src/lua/lctype.h
        src/lua/ldebug.h
        src/lua/ldo.h
        src/lua/lfunc.h
        src/lua/lgc.h
        src/lua/ljumptab.h
        src/lua/llex.h
        src/lua/llimits.h
        src/lua/lmem.h
        src/lua/lobject.h
        src/lua/lopcodes.h
        src/lua/lopnames.h
        src/lua/lparser.h
        src/lua/lprefix.h
        src/lua/lstate.h
        src/lua/lstring.h
        src/lua/ltable.h
        src/lua/ltm.h
        src/lua/lundump.h
        src/lua/lvm.h
        src/lua/lzio.h
        src/lua/lapi.c
        src/lua/lauxlib.c
        src/lua/lbaselib.c
        src/lua/lcode.c
        src/lua/lcorolib.c
        src/lua/lctype.c
        src/lua/ldblib.c
        src/lua/ldebug.c
        src/lua/ldo.c
        src/lua/ldump.c

        include/lua/lua.hpp
        include/lua/luaconf.h
        include/lua/lualib.h
        include/lua/lauxlib.h
        include/lua/lua.h
    )

    include_directories("include/lua")
    add_library(lua54 STATIC ${LUA_FILE})

    target_link_libraries(QSanguosha lua54)
else()
    target_link_libraries(QSanguosha PkgConfig::lua54)
endif()

if (WIN32)
    target_link_libraries(QSanguosha Qt5::WinExtras)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    configure_file(resource/QSanguosha_resource.rc.in QSanguosha_resource.rc @ONLY)
else()
    target_link_libraries(QSanguosha dl)
endif()

target_compile_definitions(QSanguosha PUBLIC -DOV_EXCLUDE_STATIC_CALLBACKS)
