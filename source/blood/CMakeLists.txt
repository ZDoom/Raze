cmake_minimum_required( VERSION 2.8.7 )

if( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
endif()

if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J" )
endif()

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../build/include" )

if (WIN32)
	include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/vpx" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/sdl2" "${ENET_INCLUDE_DIR}")
else ()
include_directories( "${ZLIB_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${GME_INCLUDE_DIR}" "${ENET_INCLUDE_DIR}" "${GDTOA_INCLUDE_DIR}")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/include
	${CMAKE_CURRENT_SOURCE_DIR}/../mact/include
	${CMAKE_CURRENT_SOURCE_DIR}/../audiolib/include
	${CMAKE_CURRENT_SOURCE_DIR}/../libsmackerdec/include
	${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/include
	${CMAKE_CURRENT_SOURCE_DIR}/../common
	${CMAKE_CURRENT_SOURCE_DIR}/../common/utility
	${CMAKE_CURRENT_SOURCE_DIR}/../common/console
	${CMAKE_CURRENT_SOURCE_DIR}/../common/textures
	${CMAKE_CURRENT_SOURCE_DIR}/../platform
)


if (WIN32)
set( PLAT_SOURCES
	src/startwin.game.cpp
	)
	
	
set( PCH_SOURCES
	src/actor.cpp
	src/ai.cpp
	src/aibat.cpp
	src/aibeast.cpp
	src/aiboneel.cpp
	src/aiburn.cpp
	src/aicaleb.cpp
	src/aicerber.cpp
	src/aicult.cpp
	src/aigarg.cpp
	src/aighost.cpp
	src/aigilbst.cpp
	src/aihand.cpp
	src/aihound.cpp
	src/aiinnoc.cpp
	src/aipod.cpp
	src/airat.cpp
	src/aispid.cpp
	src/aitchern.cpp
	src/aiunicult.cpp
	src/aizomba.cpp
	src/aizombf.cpp
	src/asound.cpp
	src/barf.cpp
	src/blood.cpp
	src/callback.cpp
	src/choke.cpp
	src/common.cpp
	src/config.cpp
	src/controls.cpp
	src/credits.cpp
	src/db.cpp
	src/demo.cpp
	src/dude.cpp
	src/endgame.cpp
	src/eventq.cpp
	src/fire.cpp
	src/fx.cpp
	src/gamemenu.cpp
	src/gameutil.cpp
	src/getopt.cpp
	src/gib.cpp
	src/globals.cpp
	src/inifile.cpp
	src/iob.cpp
	src/levels.cpp
	src/loadsave.cpp
	src/map2d.cpp
	src/menu.cpp
	src/messages.cpp
	src/mirrors.cpp
	src/misc.cpp
	src/network.cpp
	src/osdcmd.cpp
	src/player.cpp
	src/qav.cpp
	src/qheap.cpp
	src/replace.cpp
	src/resource.cpp
	src/screen.cpp
	src/sectorfx.cpp
	src/seq.cpp
	src/sfx.cpp
	src/sound.cpp
	src/tile.cpp
	src/trig.cpp
	src/triggers.cpp
	src/view.cpp
	src/warp.cpp
	src/weapon.cpp
)

endif()

if( MSVC )
	enable_precompiled_headers( ../g_pch.h PCH_SOURCES )
	# The original Build code was written with unsigned chars and unfortunately they still haven't been eliminated entirely.
	# All other code should stay with signed chars. What a mess... :(
	#set_source_files_properties( ${PCH_SOURCES} PROPERTIES COMPILE_FLAGS "/J" )
else()
	# Temporary solution for compilers other than MSVC
	set_source_files_properties( ${PCH_SOURCES} PROPERTIES COMPILE_FLAGS "-include g_pch.h" )
endif() 

file( GLOB HEADER_FILES
	src/*.h
	)
add_library( blood STATIC
	${HEADER_FILES}
	${PCH_SOURCES}
	${PLAT_SOURCES}
	)


