cmake_minimum_required( VERSION 3.1.0 )

require_stricmp()
require_strnicmp()

# Build does not work with signed chars!
if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J" )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funsigned-char -Wno-missing-braces -Wno-char-subscripts -Wno-unused-variable" )
endif()

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../build/include" )

if (WIN32)
	include_directories( "${ZLIB_INCLUDE_DIR}" "${ZMUSIC_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../platform/windows/include/vpx")
else ()
	include_directories( "${ZLIB_INCLUDE_DIR}" "${ZMUSIC_INCLUDE_DIR}")
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/include
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
	${CMAKE_CURRENT_SOURCE_DIR}/../common/input
	${CMAKE_CURRENT_SOURCE_DIR}/../platform
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/gl_load
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/gl/system
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/gl/renderer
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/gl/shaders
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/hwrenderer/data
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/hwrenderer/postprocessing
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering/hwrenderer/utility
	${CMAKE_CURRENT_SOURCE_DIR}/../common/rendering
)


set( NOT_COMPILED_SOURCE_FILES
)

set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )

set( PCH_SOURCES
	src/actors.cpp
	src/anim.cpp
	src/cheats.cpp
	src/cmdline.cpp
	src/common.cpp
	src/config.cpp
	src/d_menu.cpp
	src/demo.cpp
	src/game.cpp
	src/gamedef.cpp
	src/gameexec.cpp
	src/global.cpp
	src/namesdyn.cpp
	src/net.cpp
	src/osdcmds.cpp
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
add_library( rr STATIC
	${HEADER_FILES}
	${PCH_SOURCES}
	${NOT_COMPILED_SOURCE_FILES}
	)
