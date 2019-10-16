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
	src/anim.cpp
	#src/bldscript.cpp
	src/border.cpp
	src/break.cpp
	#src/brooms.cpp
	src/bunny.cpp
	src/cache.cpp
	src/cheats.cpp
	src/colormap.cpp
	src/common.cpp
	src/config.cpp
	src/console.cpp
	src/coolg.cpp
	src/coolie.cpp
	src/copysect.cpp
	src/demo.cpp
	src/draw.cpp
	src/eel.cpp
	src/game.cpp
	src/girlninj.cpp
	src/goro.cpp
	src/grpscan.cpp
	src/hornet.cpp
	src/interp.cpp
	src/interpsh.cpp
	src/inv.cpp
	#src/jbhlp.cpp
	src/jplayer.cpp
	src/jsector.cpp
	src/jweapon.cpp
	src/lava.cpp
	src/light.cpp
	src/mclip.cpp
	src/mdastr.cpp
	src/menus.cpp
	src/miscactr.cpp
	src/morph.cpp
	src/network.cpp
	src/ninja.cpp
	src/panel.cpp
	src/player.cpp
	src/predict.cpp
	src/quake.cpp
	src/ripper.cpp
	src/ripper2.cpp
	src/rooms.cpp
	src/rotator.cpp
	src/rts.cpp
	src/save.cpp
	src/saveable.cpp
	src/scrip2.cpp
	src/sector.cpp
	src/serp.cpp
	src/setup.cpp
	src/skel.cpp
	src/skull.cpp
	src/slidor.cpp
	src/sounds.cpp
	src/spike.cpp
	src/sprite.cpp
	src/sumo.cpp
	src/swconfig.cpp
	src/sync.cpp
	src/text.cpp
	src/track.cpp
	src/vator.cpp
	src/vis.cpp
	src/wallmove.cpp
	src/warp.cpp
	src/weapon.cpp
	src/zilla.cpp
	src/zombie.cpp
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
add_library( sw STATIC
	${HEADER_FILES}
	${PCH_SOURCES}
	${PLAT_SOURCES}
	)


