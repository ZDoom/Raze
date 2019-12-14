cmake_minimum_required( VERSION 3.1.0 )

if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J" )
endif()

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../build/include" )

if (WIN32)
	include_directories( "${ZLIB_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/vpx" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/sdl2" )
else ()
include_directories( "${ZLIB_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${GDTOA_INCLUDE_DIR}")
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
	${CMAKE_CURRENT_SOURCE_DIR}/../common/fonts
	${CMAKE_CURRENT_SOURCE_DIR}/../common/2d
	${CMAKE_CURRENT_SOURCE_DIR}/../common/music
	${CMAKE_CURRENT_SOURCE_DIR}/../platform
)



#set( NOT_COMPILED_SOURCE_FILES
#	src/gamestructures.cpp
#)
	
#set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )
	
set( PCH_SOURCES
	src/aistuff.cpp
	src/anims.cpp
	src/anubis.cpp
	src/bubbles.cpp
	src/bullet.cpp
	src/cd.cpp
	src/cdaudio.cpp
	src/cdrom.cpp
	src/config.cpp
	src/enginesubs.cpp
	src/exhumed.cpp
	src/exscript.cpp
	src/fish.cpp
	src/grenade.cpp
	src/gun.cpp
	src/init.cpp
	src/input.cpp
	src/items.cpp
	src/lavadude.cpp
	src/light.cpp
	src/lighting.cpp
	src/lion.cpp
	src/main.cpp
	src/map.cpp
	src/menu.cpp
	src/mono.cpp
	src/move.cpp
	src/movie.cpp
	src/mummy.cpp
	src/network.cpp
	src/object.cpp
	src/osdcmds.cpp
	src/paul.cpp
	src/player.cpp
	src/queen.cpp
	src/ra.cpp
	src/random.cpp
	src/rat.cpp
	src/record.cpp
	src/rex.cpp
	src/roach.cpp
	src/runlist.cpp
	src/save.cpp
	src/scorp.cpp
	src/sequence.cpp
	src/serial.cpp
	src/set.cpp
	src/snake.cpp
	src/sound.cpp
	src/spider.cpp
	src/status.cpp
	src/stream.cpp
	src/switch.cpp
	src/text2.cpp
	src/timer.cpp
	src/trigdat.cpp
	src/view.cpp
	src/wasp.cpp
	src/d_menu.cpp
)

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
add_library( exhumed STATIC
	${HEADER_FILES}
	${PCH_SOURCES}
	#${NOT_COMPILED_SOURCE_FILES}
	)

		
