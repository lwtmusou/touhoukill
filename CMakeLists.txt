# CMake 3.20 is needed for USE_SWIG_DEPENDENCIES
cmake_minimum_required(VERSION 3.20)

if ( NOT ( ( CMAKE_GENERATOR MATCHES "Makefiles" ) OR ( CMAKE_GENERATOR MATCHES "Ninja" ) ) )
    message(FATAL_ERROR "We only support makefile and ninja generator.")
endif()

set (PCH true)

# temporary for testing
set (BUILD_SHARED_LIBS true)

if (BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE true)
endif()

project(QSanguosha
    VERSION 0.12.0
    )

set(VERSION_DATE_QSanguosha 20211222)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

set(QSANGUOSHA_QT5_COMPONENTS Core Widgets Network LinguistTools)
if (WIN32)
    set(QSANGUOSHA_QT5_COMPONENTS ${QSANGUOSHA_QT5_COMPONENTS} WinExtras)
endif()
set(QSANGUOSHA_QT6_COMPONENTS Core Widgets Network LinguistTools)

find_package(Qt6 COMPONENTS ${QSANGUOSHA_QT6_COMPONENTS} OPTIONAL_COMPONENTS Multimedia) # Locate the position of Qt6.
find_package(Qt5 COMPONENTS ${QSANGUOSHA_QT5_COMPONENTS} OPTIONAL_COMPONENTS Multimedia) # Locate the position of Qt5.

if (NOT (Qt5_FOUND OR Qt6_FOUND))
    message(FATAL_ERROR "at least one of Qt 5 and Qt 6 is needed")
endif()

find_program(SWIG_EXECUTABLE swig)
if (CMAKE_HOST_WIN32)
    find_program(SWIG_EXECUTABLE swig.exe ${CMAKE_CURRENT_LIST_DIR}/tools/swig)
endif()

set(SWIG_USE_SWIG_DEPENDENCIES true)
set(CMAKE_SWIG_FLAGS "-nodefaultctor")

find_package(SWIG COMPONENTS lua REQUIRED)
include(UseSWIG)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    # Should we use "lua>=5.4 to indicate a lua 5.4? Lua is not even source compatible between minor versions!"
    pkg_search_module(lua54 IMPORTED_TARGET GLOBAL lua-5.4 lua5.4 lua54)
endif()

set_property(SOURCE src/swig/sgs.i
    PROPERTY
    CPLUSPLUS ON
    )

set (QSanguosha_Core_SRCS
    src/core/card.cpp
    src/core/CardFace.cpp
    src/core/engine.cpp
    src/core/game-logic.cpp
    src/core/general.cpp
    src/core/lua-wrapper.cpp
    src/core/package.cpp
    src/core/player.cpp
    src/core/protocol.cpp
    src/core/RoomObject.cpp
    src/core/skill.cpp
    src/core/trigger.cpp
    src/core/structs.cpp
    src/core/serverinfostruct.cpp
    src/core/util.cpp
    src/core/mode.cpp
    src/core/sgs_ex.cpp
    src/core/exppattern.cpp
    src/core/jsonutils.cpp
    )

set (QSanguosha_Core_MOCS
    src/core/card.h
    src/core/CardFace.h
    src/core/engine.h
    src/core/game-logic.h
    src/core/general.h
    src/core/lua-wrapper.h
    src/core/package.h
    src/core/player.h
    src/core/protocol.h
    src/core/RoomObject.h
    src/core/skill.h
    src/core/trigger.h
    src/core/structs.h
    src/core/serverinfostruct.h
    src/core/util.h
    src/core/global.h
    src/core/mode.h
    src/core/exppattern.h
    src/core/qsgscore.h
    src/core/jsonutils.h
    )

set (QSanguosha_Server_SRCS
    src/server/server.cpp
    src/server/qsgsmultiserver.cpp
    src/server/serverconfig.cpp
)

set (QSanguosha_Server_MOCS
    src/server/server.h
    src/server/qsgsmultiserver.h
    src/server/serverconfig.h
)

set (QSanguosha_SRCS
    src/util/settings.cpp
    src/audio/audio.cpp
    src/client/aux-skills.cpp
    src/client/client.cpp
    src/legacycore/legacyjson.cpp
    src/legacycore/legacyprotocol.cpp
    src/legacycore/legacystructs.cpp
    src/legacycore/legacyutil.cpp
    src/dialog/serverinfowidget.cpp
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
    src/dialog/serverdialog.cpp
    src/main.cpp
    src/legacyserver/legacygamerule.cpp
    src/legacyserver/legacyroom.cpp
    src/legacyserver/legacyroomthread.cpp
    src/legacyserver/legacyserver.cpp
    src/legacyserver/legacyserverplayer.cpp
    src/legacyserver/legacysocket.cpp
    src/legacyserver/legacydetector.cpp
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
    src/util/recorder.cpp
)

set (QSanguosha_UIS
    src/dialog/cardoverview.ui
    src/dialog/configdialog.ui
    src/dialog/generaloverview.ui
    src/dialog/mainwindow.ui
)

set(TS_FILES builds/sanguosha.ts)

set (QSanguosha_MOCS
    src/util/settings.h
    src/audio/audio.h
    src/client/aux-skills.h
    src/client/client.h
    src/legacycore/legacyjson.h
    src/legacycore/legacyprotocol.h
    src/legacycore/legacystructs.h
    src/legacycore/legacyutil.h
    src/dialog/serverinfowidget.h
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
    src/dialog/serverdialog.h
    src/legacyserver/legacygamerule.h
    src/legacyserver/legacyroom.h
    src/legacyserver/legacyroomthread.h
    src/legacyserver/legacyserver.h
    src/legacyserver/legacyserverplayer.h
    src/legacyserver/legacysocket.h
    src/legacyserver/legacydetector.h
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
    src/util/recorder.h
)

if (PCH)
    set (QSanguosha_MOCS
        ${QSanguosha_MOCS}
        src/pch.h
        )
endif()

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(QSanguosha_SRCS ${QSanguosha_SRCS} QSanguosha_resource.rc)
    set(QSanguosha_Icon_RC ${CMAKE_CURRENT_LIST_DIR}/resource/icon/sgs.ico)
    configure_file(resource/QSanguosha_resource.rc.in QSanguosha_resource.rc @ONLY)
endif()

if (lua54_FOUND)
    add_library(QSanguoshaLibs::lua54 ALIAS PkgConfig::lua54)
else()
    set(QSGS_LUAVERSION 5.4.4)

    set(QSGS_LUA_FILE
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lfunc.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lgc.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/linit.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/liolib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/llex.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lmathlib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lmem.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/loadlib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lobject.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lopcodes.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/loslib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lparser.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lstate.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lstring.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lstrlib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ltable.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ltablib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ltm.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lundump.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lutf8lib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lvm.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lzio.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lapi.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lcode.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lctype.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldebug.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldo.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lfunc.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lgc.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ljumptab.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/llex.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/llimits.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lmem.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lobject.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lopcodes.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lopnames.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lparser.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lprefix.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lstate.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lstring.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ltable.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ltm.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lundump.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lvm.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lzio.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lapi.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lauxlib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lbaselib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lcode.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lcorolib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lctype.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldblib.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldebug.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldo.c
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/ldump.c

        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lua.hpp
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/luaconf.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lualib.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lauxlib.h
        src/3rdparty/lua-${QSGS_LUAVERSION}/src/lua.h
    )

    add_library(lua54 STATIC ${QSGS_LUA_FILE})
    target_include_directories(lua54 PUBLIC "src/3rdparty/lua-${QSGS_LUAVERSION}/src")
    if(APPLE)
        target_compile_definitions(lua54 PRIVATE LUA_USE_MACOSX)
    elseif(NOT WIN32)
        target_compile_definitions(lua54 PRIVATE LUA_USE_LINUX)
    endif()

    add_library(QSanguoshaLibs::lua54 ALIAS lua54)
endif()

if (Qt5Multimedia_FOUND OR Qt6Multimedia_FOUND)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(oggvorbisfile IMPORTED_TARGET GLOBAL ogg>=1.0.0 vorbis>=1.0.0 vorbisfile>=1.0.0)
    endif()

    if (oggvorbisfile_FOUND)
        add_library(QSanguoshaLibs::oggvorbisfile ALIAS PkgConfig::oggvorbisfile)
    else()
        set(QSGS_OGGVERSION 1.3.5)
        set(QSGS_VORBISVERSION 1.3.7)

        set(QSGS_OGG_FILE
            # OGG files
            src/3rdparty/libogg-${QSGS_OGGVERSION}/src/bitwise.c
            src/3rdparty/libogg-${QSGS_OGGVERSION}/src/crctable.h
            src/3rdparty/libogg-${QSGS_OGGVERSION}/src/framing.c
            src/3rdparty/libogg-${QSGS_OGGVERSION}/include/ogg/ogg.h
            src/3rdparty/libogg-${QSGS_OGGVERSION}/include/ogg/os_types.h

            # Vorbis files
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/analysis.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/bitrate.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/block.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/codebook.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/envelope.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/floor0.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/floor1.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/info.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lookup.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lpc.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lsp.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/mapping0.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/mdct.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/psy.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/registry.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/res0.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/sharedbook.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/smallft.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/synthesis.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/vorbisfile.c
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/window.c

            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/backends.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/bitrate.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/codebook.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/codec_internal.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/envelope.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/highlevel.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lookup.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lookup_data.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lpc.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/lsp.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/masking.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/mdct.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/misc.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/os.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/psy.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/registry.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/scales.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/smallft.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/lib/window.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/include/vorbis/codec.h
            src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/include/vorbis/vorbisfile.h

        )

        add_library(oggvorbisfile STATIC ${QSGS_OGG_FILE})
        target_include_directories(oggvorbisfile PUBLIC "src/3rdparty/libogg-${QSGS_OGGVERSION}/include")
        target_include_directories(oggvorbisfile PUBLIC "src/3rdparty/libvorbis-${QSGS_VORBISVERSION}/include")

        if (MSVC)
            target_compile_options(oggvorbisfile BEFORE PRIVATE "-wd4244" PRIVATE "-wd4305" PRIVATE "-wd4267")
        endif()
        add_library(QSanguoshaLibs::oggvorbisfile ALIAS oggvorbisfile)
    endif()
endif()

if (Qt5_FOUND)
    swig_add_library(q5sanguosha_wrap
        TYPE STATIC
        LANGUAGE lua
        SOURCES src/swig/sgs.i
        )

    target_compile_definitions(q5sanguosha_wrap PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x60000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    # ONLY FOR THEIR INCLUDE DIRECTORIES! Compile fails if we don't add the link libraries.
    # Seems weird since we don't even link the libraries here
    target_link_libraries(q5sanguosha_wrap QSanguoshaLibs::lua54 Qt5::Core)
    target_include_directories(q5sanguosha_wrap PRIVATE "src/core")
    set_property(TARGET q5sanguosha_wrap PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
    set_property(TARGET q5sanguosha_wrap PROPERTY SWIG_COMPILE_DEFINITIONS QT_MAJOR_VERSION=5)

    add_library(Q5SgsCore ${QSanguosha_Core_SRCS} ${QSanguosha_Core_MOCS} lua/lua.qrc )

    set_property(TARGET Q5SgsCore PROPERTY AUTOMOC ON)
    set_property(TARGET Q5SgsCore PROPERTY AUTOUIC ON)
    set_property(TARGET Q5SgsCore PROPERTY AUTORCC ON)

    target_compile_definitions(Q5SgsCore PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x60000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    target_link_libraries(Q5SgsCore Qt5::Core q5sanguosha_wrap QSanguoshaLibs::lua54)
    target_include_directories(Q5SgsCore PUBLIC "src/core")

    if (NOT WIN32)
        target_link_libraries(Q5SgsCore dl)
    endif()

    set_target_properties(Q5SgsCore PROPERTIES VERSION ${QSanguosha_VERSION})

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(q5sanguosha_wrap PUBLIC -DQSGS_STATIC)
        target_compile_definitions(Q5SgsCore PUBLIC -DQSGS_STATIC)
    else()
        target_compile_definitions(q5sanguosha_wrap PRIVATE -DBUILD_QSGSCORE)
        target_compile_definitions(Q5SgsCore PRIVATE -DBUILD_QSGSCORE)

        set_target_properties(Q5SgsCore PROPERTIES SOVERSION 0.12)
    endif()

    add_executable(Q5SgsServer ${QSanguosha_Server_SRCS} ${QSanguosha_Server_MOCS} )

    set_property(TARGET Q5SgsServer PROPERTY AUTOMOC ON)
    set_property(TARGET Q5SgsServer PROPERTY AUTOUIC ON)
    set_property(TARGET Q5SgsServer PROPERTY AUTORCC ON)

    target_link_libraries(Q5SgsServer Qt5::Network Q5SgsCore)
    target_compile_definitions(Q5SgsServer PUBLIC -DQT_NO_CAST_FROM_ASCII)

    if (NOT ANDROID)
        add_executable(Q5Sanguosha WIN32 MACOSX_BUNDLE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} )
    else()
        add_library(Q5Sanguosha MODULE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} )
    endif()

    set_property(TARGET Q5Sanguosha PROPERTY AUTOMOC ON)
    set_property(TARGET Q5Sanguosha PROPERTY AUTOUIC ON)
    set_property(TARGET Q5Sanguosha PROPERTY AUTORCC ON)

    target_include_directories(Q5Sanguosha
        PUBLIC "src/audio"
        PUBLIC "src/client"
        PUBLIC "src/dialog"
        PUBLIC "src/package"
        PUBLIC "src/legacyserver"
        PUBLIC "src/legacycore"
        PUBLIC "src/ui"
        PUBLIC "src/util"
        PUBLIC "src"
    )

    if (WIN32)
        target_link_libraries(Q5Sanguosha Qt5::WinExtras)
    endif()

    if (Qt5Multimedia_FOUND)
        target_compile_definitions(Q5Sanguosha PUBLIC AUDIO_SUPPORT)
        target_link_libraries(Q5Sanguosha QSanguoshaLibs::oggvorbisfile Qt5::Multimedia)
    endif()

    target_link_libraries(Q5Sanguosha Qt5::Widgets Qt5::Gui Qt5::Network Q5SgsCore)

    target_compile_definitions(Q5Sanguosha PUBLIC -DOV_EXCLUDE_STATIC_CALLBACKS PUBLIC -DQT_NO_CAST_FROM_ASCII)
    if (PCH)
        target_precompile_headers(q5sanguosha_wrap PUBLIC src/core/qsgscore.h)
        target_precompile_headers(Q5SgsCore PUBLIC src/core/qsgscore.h)
        target_precompile_headers(Q5Sanguosha PUBLIC src/pch.h)
    endif()
endif()

if (Qt6_FOUND)
    swig_add_library(qsanguosha_wrap
        TYPE STATIC
        LANGUAGE lua
        SOURCES src/swig/sgs.i
        )

    target_compile_definitions(qsanguosha_wrap PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x70000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    # ONLY FOR THEIR INCLUDE DIRECTORIES! Compile fails if we don't add the link libraries.
    # Seems weird since we don't even link the libraries here
    target_link_libraries(qsanguosha_wrap QSanguoshaLibs::lua54 Qt6::Core)
    target_include_directories(qsanguosha_wrap PRIVATE "src/core")
    set_property(TARGET qsanguosha_wrap PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
    set_property(TARGET qsanguosha_wrap PROPERTY SWIG_COMPILE_DEFINITIONS QT_MAJOR_VERSION=6)

    qt6_add_library(QSgsCore ${QSanguosha_Core_SRCS} ${QSanguosha_Core_MOCS} lua/lua.qrc )

    set_property(TARGET QSgsCore PROPERTY AUTOMOC ON)
    set_property(TARGET QSgsCore PROPERTY AUTOUIC ON)
    set_property(TARGET QSgsCore PROPERTY AUTORCC ON)

    target_compile_definitions(QSgsCore PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x70000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    target_link_libraries(QSgsCore Qt6::Core qsanguosha_wrap QSanguoshaLibs::lua54)
    target_include_directories(QSgsCore PUBLIC "src/core")

    if (NOT WIN32)
        target_link_libraries(QSgsCore dl)
    endif()

    set_target_properties(QSgsCore PROPERTIES VERSION ${QSanguosha_VERSION})

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(qsanguosha_wrap PUBLIC -DQSGS_STATIC)
        target_compile_definitions(QSgsCore PUBLIC -DQSGS_STATIC)
    else()
        target_compile_definitions(qsanguosha_wrap PRIVATE -DBUILD_QSGSCORE)
        target_compile_definitions(QSgsCore PRIVATE -DBUILD_QSGSCORE)

        set_target_properties(QSgsCore PROPERTIES SOVERSION 0.12)
    endif()

    add_executable(QSgsServer ${QSanguosha_Server_SRCS} ${QSanguosha_Server_MOCS} )

    set_property(TARGET QSgsServer PROPERTY AUTOMOC ON)
    set_property(TARGET QSgsServer PROPERTY AUTOUIC ON)
    set_property(TARGET QSgsServer PROPERTY AUTORCC ON)

    target_link_libraries(QSgsServer Qt6::Network QSgsCore)
    target_compile_definitions(QSgsServer PUBLIC -DQT_NO_CAST_FROM_ASCII)

    qt6_add_translation(QM_FILES6 ${TS_FILES})
    qt6_add_executable(QSanguosha WIN32 MACOSX_BUNDLE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} ${QM_FILES6} )

    set_property(TARGET QSanguosha PROPERTY AUTOMOC ON)
    set_property(TARGET QSanguosha PROPERTY AUTOUIC ON)
    set_property(TARGET QSanguosha PROPERTY AUTORCC ON)

    target_include_directories(QSanguosha
        PUBLIC "src/audio"
        PUBLIC "src/client"
        PUBLIC "src/dialog"
        PUBLIC "src/package"
        PUBLIC "src/legacyserver"
        PUBLIC "src/legacycore"
        PUBLIC "src/ui"
        PUBLIC "src/util"
        PUBLIC "src"
    )

    if (Qt6Multimedia_FOUND)
        target_compile_definitions(QSanguosha PUBLIC AUDIO_SUPPORT)
        target_link_libraries(QSanguosha PUBLIC QSanguoshaLibs::oggvorbisfile Qt6::Multimedia)
    endif()

    target_link_libraries(QSanguosha PUBLIC Qt6::Widgets Qt6::Gui Qt6::Network QSgsCore)

    target_compile_definitions(QSanguosha PUBLIC -DOV_EXCLUDE_STATIC_CALLBACKS PUBLIC -DQT_NO_CAST_FROM_ASCII)
    if (PCH)
        target_precompile_headers(qsanguosha_wrap PUBLIC src/core/qsgscore.h)
        target_precompile_headers(QSgsCore PUBLIC src/core/qsgscore.h)
        target_precompile_headers(QSanguosha PUBLIC src/pch.h)
    endif()
endif()
