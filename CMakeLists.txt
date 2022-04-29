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
    src/core/game-state.cpp
    src/core/general.cpp
    src/core/json.cpp
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
    )

set (QSanguosha_Core_MOCS
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
    src/core/skill.h
    src/core/trigger.h
    src/core/structs.h
    src/core/serverinfostruct.h
    src/core/util.h
    src/core/global.h
    src/core/mode.h
    src/core/exppattern.h
    src/core/qsgscore.h
    )

set (QSanguosha_SRCS
    src/util/settings.cpp
    src/audio/audio.cpp
    src/client/aux-skills.cpp
    src/client/client.cpp
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
    src/main.cpp
    src/server/gamerule.cpp
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
    src/server/gamerule.h
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

    add_library(Q5SgsCore ${QSanguosha_Core_SRCS} ${QSanguosha_Core_MOCS} lua/lua.qrc )

    set_property(TARGET Q5SgsCore PROPERTY AUTOMOC ON)
    set_property(TARGET Q5SgsCore PROPERTY AUTOUIC ON)
    set_property(TARGET Q5SgsCore PROPERTY AUTORCC ON)

    target_compile_definitions(Q5SgsCore PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x60000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    target_link_libraries(Q5SgsCore Qt5::Core q5sanguosha_wrap QSanguoshaLibs::lua54)

    if (NOT WIN32)
        target_link_libraries(Q5SgsCore dl)
    endif()

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(q5sanguosha_wrap PUBLIC -DQSGS_STATIC)
        target_compile_definitions(Q5SgsCore PUBLIC -DQSGS_STATIC)
    else()
        target_compile_definitions(q5sanguosha_wrap PRIVATE -DBUILD_QSGSCORE)
        target_compile_definitions(Q5SgsCore PRIVATE -DBUILD_QSGSCORE)

        set_target_properties(Q5SgsCore PROPERTIES VERSION ${QSanguosha_VERSION} SOVERSION 0.12)
    endif()

    if (NOT ANDROID)
        add_executable(Q5Sanguosha WIN32 MACOSX_BUNDLE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} )
    else()
        add_library(Q5Sanguosha MODULE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} )
    endif()

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(Q5Sanguosha PUBLIC -DQSGS_STATIC)
    endif()

    set_property(TARGET Q5Sanguosha PROPERTY AUTOMOC ON)
    set_property(TARGET Q5Sanguosha PROPERTY AUTOUIC ON)
    set_property(TARGET Q5Sanguosha PROPERTY AUTORCC ON)

    if (WIN32)
        target_link_libraries(Q5Sanguosha Qt5::WinExtras)
    endif()

    if (Qt5Multimedia_FOUND)
        target_compile_definitions(Q5Sanguosha PUBLIC AUDIO_SUPPORT)
        target_link_libraries (Q5Sanguosha QSanguoshaLibs::oggvorbisfile Qt5::Multimedia)
    endif()

    target_link_libraries (Q5Sanguosha Qt5::Widgets Qt5::Gui Qt5::Network Q5SgsCore)

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

    qt6_add_library(QSgsCore ${QSanguosha_Core_SRCS} ${QSanguosha_Core_MOCS} lua/lua.qrc )

    set_property(TARGET QSgsCore PROPERTY AUTOMOC ON)
    set_property(TARGET QSgsCore PROPERTY AUTOUIC ON)
    set_property(TARGET QSgsCore PROPERTY AUTORCC ON)

    target_compile_definitions(QSgsCore PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x70000 VERSION=${QSanguosha_VERSION} VERSIONDATE=${VERSION_DATE_QSanguosha})
    target_link_libraries(QSgsCore Qt6::Core qsanguosha_wrap QSanguoshaLibs::lua54)

    if (NOT WIN32)
        target_link_libraries(QSgsCore dl)
    endif()

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(qsanguosha_wrap PUBLIC -DQSGS_STATIC)
        target_compile_definitions(QSgsCore PUBLIC -DQSGS_STATIC)
    else()
        target_compile_definitions(qsanguosha_wrap PRIVATE -DBUILD_QSGSCORE)
        target_compile_definitions(QSgsCore PRIVATE -DBUILD_QSGSCORE)

        set_target_properties(QSgsCore PROPERTIES VERSION ${QSanguosha_VERSION} SOVERSION 0.12)
    endif()

    qt6_add_translation(QM_FILES6 ${TS_FILES})
    qt6_add_executable(QSanguosha WIN32 MACOSX_BUNDLE ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} ${QM_FILES6} )

    if (NOT BUILD_SHARED_LIBS)
        target_compile_definitions(QSanguosha PUBLIC -DQSGS_STATIC)
    endif()

    set_property(TARGET QSanguosha PROPERTY AUTOMOC ON)
    set_property(TARGET QSanguosha PROPERTY AUTOUIC ON)
    set_property(TARGET QSanguosha PROPERTY AUTORCC ON)

    if (Qt6Multimedia_FOUND)
        target_compile_definitions(QSanguosha PUBLIC AUDIO_SUPPORT)
        target_link_libraries (QSanguosha PUBLIC QSanguoshaLibs::oggvorbisfile Qt6::Multimedia)
    endif()

    target_link_libraries (QSanguosha PUBLIC Qt6::Widgets Qt6::Gui Qt6::Network QSgsCore)

    target_compile_definitions(QSanguosha PUBLIC -DOV_EXCLUDE_STATIC_CALLBACKS PUBLIC -DQT_NO_CAST_FROM_ASCII)
    if (PCH)
        target_precompile_headers(qsanguosha_wrap PUBLIC src/core/qsgscore.h)
        target_precompile_headers(QSgsCore PUBLIC src/core/qsgscore.h)
        target_precompile_headers(QSanguosha PUBLIC src/pch.h)
    endif()
endif()
