cmake_minimum_required(VERSION 3.12)

project(QSanguosha)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#set( CMAKE_BUILD_TYPE Release )

# add_definitions(-Wall)

find_package(Qt5 COMPONENTS Widgets Network LinguistTools Multimedia REQUIRED) # Locate the position of Qt5.

if (WIN32)
    find_package(Qt5 COMPONENTS WinExtras REQUIRED)
else()
    add_compile_definitions(LUA_USE_LINUX)
endif()

include_directories(
	"src/audio"
	"src/client"
	"src/core"
	"src/dialog"
	"src/lua"
	"src/package"
	"src/server"
	"src/ui"
	"src/util"
	"src"
	"include"
	"include/vld"
)

add_compile_definitions(AUDIO_SUPPORT)

if (WIN32)
    set(SWIG_EXECUTABLE ${CMAKE_CURRENT_LIST_DIR}/tools/swig/swig.exe)
else()
    set(SWIG_EXECUTABLE swig)
endif()

add_custom_command(OUTPUT sanguosha_wrap.cpp
                   COMMAND ${SWIG_EXECUTABLE} ARGS -c++ -lua -cppext cpp -o sanguosha_wrap.cpp ${CMAKE_CURRENT_LIST_DIR}/swig/sanguosha.i
                   MAIN_DEPENDENCY swig/sanguosha.i
                   DEPENDS swig/sanguosha.i
                   COMMENT "Generating sanguosha_wrap.cpp ....")

set (QSanguosha_SRCS
        sanguosha_wrap.cpp

	src/audio/audio.cpp
	src/audio/OggFile.cpp
	src/client/aux-skills.cpp
	src/client/client.cpp
	src/client/clientplayer.cpp
        src/client/clientstruct.cpp
	src/core/card.cpp
	src/core/engine.cpp
	src/core/general.cpp
	src/core/json.cpp
	src/core/lua-wrapper.cpp
	src/core/player.cpp
	src/core/protocol.cpp
	src/core/record-analysis.cpp
	src/core/RoomObject.cpp
	src/core/settings.cpp
	src/core/skill.cpp
	src/core/structs.cpp
	src/core/util.cpp
	src/core/WrappedCard.cpp
	src/dialog/AboutUs.cpp
	src/dialog/cardoverview.cpp
	src/dialog/choosegeneraldialog.cpp
	src/dialog/configdialog.cpp
	src/dialog/connectiondialog.cpp
	src/dialog/distanceviewdialog.cpp
	src/dialog/generaloverview.cpp
	src/dialog/updatedialog.cpp
	src/dialog/mainwindow.cpp
	src/dialog/roleassigndialog.cpp
	src/package/exppattern.cpp
	src/package/maneuvering.cpp
	src/package/package.cpp
	src/package/protagonist.cpp
	src/package/standard-cards.cpp
	src/package/standard.cpp
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
	src/package/hegemonyGeneral.cpp
	src/server/ai.cpp
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
	src/ui/clientlogbox.cpp
	src/ui/dashboard.cpp
	src/ui/GenericCardContainerUI.cpp
	src/ui/graphicspixmaphoveritem.cpp
	src/ui/heroskincontainer.cpp
	src/ui/indicatoritem.cpp
	src/ui/magatamasItem.cpp
	src/ui/photo.cpp
	src/ui/pixmapanimation.cpp
	src/ui/choosegeneralbox.cpp
	src/ui/qsanbutton.cpp
	src/ui/QSanSelectableItem.cpp
	src/ui/rolecombobox.cpp
	src/ui/roomscene.cpp
	src/ui/SkinBank.cpp
	src/ui/skinitem.cpp
	src/ui/sprite.cpp
	src/ui/startscene.cpp
	src/ui/TablePile.cpp
	src/ui/TimedProgressBar.cpp
	src/ui/uiUtils.cpp
	src/ui/sgswindow.cpp
	src/util/detector.cpp
	src/util/nativesocket.cpp
	src/util/recorder.cpp
	src/main.cpp
	src/ui/choosetriggerorderbox.cpp
	src/ui/graphicsbox.cpp
	src/ui/lightboxanimation.cpp
	src/ui/chooseoptionsbox.cpp
	src/ui/playercardbox.cpp
	src/package/testCard.cpp
	src/package/hegemonyCard.cpp
	src/package/playground.cpp
	src/ui/hegemonyrolecombobox.cpp
	src/lua/lzio.c
	src/lua/lvm.c
	src/lua/lundump.c
	src/lua/ltm.c
	src/lua/ltablib.c
	src/lua/ltable.c
	src/lua/lstrlib.c
	src/lua/lstring.c
	src/lua/lstate.c
	src/lua/lparser.c
	src/lua/loslib.c
	src/lua/lopcodes.c
	src/lua/lobject.c
	src/lua/loadlib.c
	src/lua/lmem.c
	src/lua/lmathlib.c
	src/lua/llex.c
	src/lua/liolib.c
	src/lua/linit.c
	src/lua/lgc.c
	src/lua/lfunc.c
	src/lua/ldump.c
	src/lua/ldo.c
	src/lua/ldebug.c
	src/lua/ldblib.c
	src/lua/lctype.c
	src/lua/lcorolib.c
	src/lua/lcode.c
	src/lua/lbitlib.c
	src/lua/lbaselib.c
	src/lua/lauxlib.c
	src/lua/lapi.c
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

set (QSanguosha_MOCS
	src/audio/audio.h
	src/audio/OggFile.h
	src/client/aux-skills.h
	src/client/client.h
	src/client/clientplayer.h
	src/client/clientstruct.h
	src/core/card.h
	src/core/engine.h
	src/core/general.h
	src/core/lua-wrapper.h
	src/core/player.h
	src/core/record-analysis.h
	src/core/settings.h
	src/core/skill.h
	src/core/WrappedCard.h
	src/dialog/AboutUs.h
	src/dialog/cardoverview.h
	src/dialog/choosegeneraldialog.h
	src/dialog/configdialog.h
	src/dialog/connectiondialog.h
	src/dialog/distanceviewdialog.h
	src/dialog/generaloverview.h
	src/dialog/updatedialog.h
	src/dialog/mainwindow.h
	src/dialog/roleassigndialog.h
	src/package/maneuvering.h
	src/package/package.h
	src/package/protagonist.h
	src/package/standard-equips.h
	src/package/standard.h
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
	src/package/hegemonyGeneral.h
	src/server/ai.h
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
	src/ui/clientlogbox.h
	src/ui/dashboard.h
	src/ui/GenericCardContainerUI.h
	src/ui/graphicspixmaphoveritem.h
	src/ui/choosegeneralbox.h
	src/ui/heroskincontainer.h
	src/ui/indicatoritem.h
	src/ui/magatamasItem.h
	src/ui/photo.h
	src/ui/pixmapanimation.h
	src/ui/qsanbutton.h
	src/ui/QSanSelectableItem.h
	src/ui/rolecombobox.h
	src/ui/roomscene.h
	src/ui/skinitem.h
	src/ui/sprite.h
	src/ui/startscene.h
	src/ui/TablePile.h
	src/ui/TimedProgressBar.h
	src/ui/sgswindow.h
	src/util/detector.h
	src/util/nativesocket.h
	src/util/recorder.h
	src/util/socket.h
	src/ui/choosetriggerorderbox.h
	src/ui/graphicsbox.h
	src/ui/lightboxanimation.h
	src/ui/hegemonyrolecombobox.h
	src/ui/chooseoptionsbox.h
	src/ui/playercardbox.h
	src/package/testCard.h
	src/package/hegemonyCard.h
	src/package/playground.h
)

set(OGG_FILE
src/ogg/scales.h
src/ogg/lpc.c
src/ogg/registry.c
src/ogg/highlevel.h
src/ogg/crctable.h
src/ogg/bitrate.c
src/ogg/envelope.c
src/ogg/window.c
src/ogg/lsp.h
src/ogg/psy.c
src/ogg/window.h
src/ogg/lookup_data.h
src/ogg/registry.h
src/ogg/info.c
src/ogg/analysis.c
src/ogg/lookup.h
src/ogg/os.h
src/ogg/backends.h
src/ogg/masking.h
src/ogg/lookup.c
src/ogg/misc.h
src/ogg/mapping0.c
src/ogg/sharedbook.c
src/ogg/mdct.h
src/ogg/smallft.c
src/ogg/codebook.h
src/ogg/psy.h
src/ogg/lsp.c
src/ogg/envelope.h
src/ogg/lpc.h
src/ogg/codebook.c
src/ogg/mdct.c
src/ogg/block.c
src/ogg/bitrate.h
src/ogg/floor0.c
src/ogg/codec_internal.h
src/ogg/smallft.h
src/ogg/synthesis.c
src/ogg/floor1.c
src/ogg/res0.c
src/ogg/vorbisfile.c

src/ogg/bitwise.c
src/ogg/framing.c
)



add_executable (QSanguosha ${QSanguosha_SRCS} ${QSanguosha_MOCS} ${QSanguosha_UIS} ${OGG_FILE})
target_link_libraries (QSanguosha Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Multimedia)

if (WIN32)
    target_link_libraries(QSanguosha Qt5::WinExtras)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    target_link_libraries(QSanguosha dl)
endif()
