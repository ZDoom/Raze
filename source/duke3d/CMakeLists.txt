cmake_minimum_required( VERSION 2.8.7 )

if( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
endif()

if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J" )
endif()

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../build/include" )

if (WIN32)
	include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/vpx" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/sdl2" )
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
	${CMAKE_CURRENT_SOURCE_DIR}/../platform
)


if (WIN32)
set( PLAT_SOURCES
	src/startwin.game.cpp
	)
endif()
	
set( NOT_COMPILED_SOURCE_FILES
	src/gamestructures.cpp
)
	
set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )
	
set( PCH_SOURCES
	src/actors.cpp
	src/anim.cpp
	src/cheats.cpp
	src/cmdline.cpp
	src/common.cpp
	src/config.cpp
	src/demo.cpp
	src/game.cpp
	src/gamedef.cpp
	src/gameexec.cpp
	src/gamevars.cpp
	src/global.cpp
	src/grpscan.cpp
	src/menus.cpp
	src/namesdyn.cpp
	src/network.cpp
	src/osdcmds.cpp
	src/osdfuncs.cpp
	src/player.cpp
	src/premap.cpp
	src/savegame.cpp
	src/sbar.cpp
	src/screens.cpp
	src/screentext.cpp
	src/sector.cpp
	src/sounds.cpp
	src/soundsdyn.cpp
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
add_library( duke3d STATIC
	${HEADER_FILES}
	${PCH_SOURCES}
	${PLAT_SOURCES}
	${NOT_COMPILED_SOURCE_FILES}
	)


