cmake_minimum_required( VERSION 3.1.0 )

include(precompiled_headers)

if( COMMAND cmake_policy )
	cmake_policy( SET CMP0003 NEW )
endif()

include( CheckCXXSourceCompiles )
include( CheckFunctionExists )
include( CheckCXXCompilerFlag )
include( CheckIncludeFile )
include( CheckIncludeFiles )
include( CheckLibraryExists )
include( FindPkgConfig )

if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
	option( NO_STRIP "Do not strip Release or MinSizeRel builds" )
	# At least some versions of Xcode fail if you strip with the linker
	# instead of the separate strip utility.
	if( APPLE )
		set( NO_STRIP ON )
	endif()
endif()

# Build does not work with signed chars!
if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J" )
else()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funsigned-char -Wno-missing-braces -Wno-char-subscripts -Wno-unused-variable" )
endif()

option( DYN_OPENAL "Dynamically load OpenAL" ON )

if( APPLE )
    option( OSX_COCOA_BACKEND "Use native Cocoa backend instead of SDL" ON )
endif()

target_architecture(TARGET_ARCHITECTURE)
message(STATUS "Architecture is ${TARGET_ARCHITECTURE}")

# Right now only 64 bit is supported.
if( ${TARGET_ARCHITECTURE} MATCHES "x86_64" )
			set( X64 64 )
endif()

if( X64 OR ${TARGET_ARCHITECTURE} MATCHES "i386" )
	add_definitions( -DARCH_IA32 )
endif()

if( NOT PROJECT_LIBRARIES )
	set( PROJECT_LIBRARIES "" )
endif()

if( WIN32 )
	if( X64 )
		set( WIN_TYPE Win64 )
		set( XBITS x64 )
	else()
		set( WIN_TYPE Win32 )
		set( XBITS x86 )
	endif()

	add_definitions( -D_WIN32 )


	set( DX_dinput8_LIBRARY dinput8 )

	set( PROJECT_LIBRARIES
		opengl32
		wsock32
		winmm
		"${DX_dinput8_LIBRARY}"
		ole32
		user32
		gdi32
		comctl32
		comdlg32
		ws2_32
		setupapi
		oleaut32
		dbghelp
		legacy_stdio_definitions

		shlwapi
		version
		imm32
		dxguid
		dsound
		winspool
		advapi32
		shell32
		SetupAPI
		uuid
		odbc32
		odbccp32

		# this local library crap needs to go away. Sadly there doesn't seem to be anything to get a working libvpx that doesn't force linking with MinGW dependencies.
		libvpx
		libcompat-to-msvc

		)




	if( NOT DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
		set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} DelayImp )
	endif()

	if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
		if( DX_dxguid_LIBRARY )
			list( APPEND PROJECT_LIBRARIES "${DX_dxguid_LIBRARY}" )
		endif()
	endif()
else()
	if( APPLE )
		set( NO_GTK ON )
		set( DYN_GTK OFF )

		# Prevent inclusion of fp.h and FixMath.h from Carbon framework
		# Declarations from these files are not used but cause the following conflicts:
		#  - redefinition of 'FixedToFloat' and 'FloatToFixed' macros
		#  - redefinition of 'pi' as different kind of symbol
		add_definitions( -D__FP__ -D__FIXMATH__ )
	else()
		option( NO_GTK "Disable GTK+ dialogs (Not applicable to Windows)" )
		option( DYN_GTK "Load GTK+ at runtime instead of compile time" ON )

		# Use GTK+ for the IWAD picker, if available.
		if( NOT NO_GTK )
			pkg_check_modules( GTK3 gtk+-3.0 )
			if( GTK3_FOUND )
				if( NOT DYN_GTK )
					set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${GTK3_LIBRARIES} )
				endif()
				include_directories( ${GTK3_INCLUDE_DIRS} )
				link_directories( ${GTK3_LIBRARY_DIRS} )
			else()
				pkg_check_modules( GTK2 gtk+-2.0 )
				if( GTK2_FOUND )
					if( NOT DYN_GTK )
						set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${GTK2_LIBRARIES} )
					endif()
					include_directories( ${GTK2_INCLUDE_DIRS} )
					link_directories( ${GTK2_LIBRARY_DIRS} )
				else()
					set( NO_GTK ON )
				endif()
			endif()
		endif()
	endif()

	if( NO_GTK )
		add_definitions( -DNO_GTK )
	elseif( DYN_GTK )
		add_definitions( -DDYN_GTK=1 )
	else()
		add_definitions( -DDYN_GTK=0 )
	endif()

	# Non-Windows version also needs SDL except native OS X backend
	if( NOT APPLE OR NOT OSX_COCOA_BACKEND )
		find_package( SDL2 REQUIRED )
		include_directories( "${SDL2_INCLUDE_DIR}" )
		set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} "${SDL2_LIBRARY}" )
	endif()

	find_path( FPU_CONTROL_DIR fpu_control.h )
	if( FPU_CONTROL_DIR )
		include_directories( ${FPU_CONTROL_DIR} )
		add_definitions( -DHAVE_FPU_CONTROL )
	endif()
endif()

if( NOT NO_OPENAL )
	if ( NOT DYN_OPENAL )	# DYN_OPENAL uses local copies of the headers.
		find_package( OpenAL )
		mark_as_advanced(CLEAR OPENAL_INCLUDE_DIR)
		if( OPENAL_INCLUDE_DIR )
			include_directories( ${OPENAL_INCLUDE_DIR} )
			mark_as_advanced(CLEAR OPENAL_LIBRARY)
			if( OPENAL_LIBRARY )
				set( PROJECT_LIBRARIES ${OPENAL_LIBRARY} ${PROJECT_LIBRARIES} )
			else()
				set( NO_OPENAL ON )
			endif()
		else()
			set( NO_OPENAL ON )
		endif()
	else()
		add_definitions( -DDYN_OPENAL )
	endif()
endif()

if( NO_OPENAL )
	add_definitions( -DNO_OPENAL=1 )
endif()


# Decide on SSE setup

# SSE only matters on 32-bit targets. We check compiler flags to know if we can do it.
if( CMAKE_SIZEOF_VOID_P MATCHES "4" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES ppc )
		CHECK_CXX_COMPILER_FLAG( "-msse2 -mfpmath=sse" CAN_DO_MFPMATH )
		CHECK_CXX_COMPILER_FLAG( -arch:SSE2 CAN_DO_ARCHSSE2 )
		if( CAN_DO_MFPMATH )
			set( SSE1_ENABLE "-msse -mfpmath=sse" )
			set( SSE2_ENABLE "-msse2 -mfpmath=sse" )
		elseif( CAN_DO_ARCHSSE2 )
			set( SSE1_ENABLE -arch:SSE )
			set( SSE2_ENABLE -arch:SSE2 )
		endif()
endif()

if( X64 )
	set( HAVE_MMX 1 )
else( X64 )
	set( SAFE_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )

	if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx")
	endif( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )

	CHECK_CXX_SOURCE_COMPILES("#include <mmintrin.h>
		int main(void) { __m64 v = _m_from_int(0); }"
		HAVE_MMX)

	set( CMAKE_CXX_FLAGS ${SAFE_CMAKE_CXX_FLAGS} )
endif( X64 )

CHECK_CXX_SOURCE_COMPILES("#include <ppl.h>
	int main() { concurrency::parallel_for(0, 1, 1, [](int) { } ); }"
	HAVE_PARALLEL_FOR)

if( NOT HAVE_PARALLEL_FOR )
	CHECK_CXX_SOURCE_COMPILES("#include <dispatch/dispatch.h>
		int main() { dispatch_apply(1, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t) { }); }"
		HAVE_DISPATCH_APPLY)
endif()

# Set up flags for MSVC
if (MSVC)
	set( CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}" )
endif (MSVC)

# Set up flags for GCC

if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
	if( PROFILE )
		set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg" )
		set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg" )
		set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pg" )
		set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -pg" )
	endif()

	#set( REL_CXX_FLAGS "-fno-rtti" )
	if( NOT PROFILE AND NOT APPLE )
		# On OS X frame pointers are required for exception handling, at least with Clang
		set( REL_CXX_FLAGS "${REL_CXX_FLAGS} -fomit-frame-pointer" )
	endif()
	set( CMAKE_CXX_FLAGS_RELEASE "${REL_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}" )
	set( CMAKE_CXX_FLAGS_MINSIZEREL "${REL_CXX_FLAGS} ${CMAKE_CXX_FLAGS_MINSIZEREL}" )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${REL_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )

	# Support for the GCC/Clang sanitizers.
	set( WITH_ASAN 0 CACHE BOOL "Enable the Address Sanitizer")
	if( NOT CMAKE_COMPILER_IS_GNUCXX )
		set( WITH_MSAN 0 CACHE BOOL "Enable the Memory Sanitizer")
	endif( NOT CMAKE_COMPILER_IS_GNUCXX )
	set( WITH_UBSAN 0 CACHE BOOL "Enable the Undefined Behavior Sanitizer")
	if( WITH_MSAN )
		if ( WITH_ASAN OR WITH_UBSAN )
			message( SEND_ERROR "You can't use MSAN with either ASAN or UBSAN." )
		endif ( WITH_ASAN OR WITH_UBSAN )
	endif( WITH_MSAN )

	set( SANITIZER_FLAG "" )
	if( WITH_ASAN )
		set( SANITIZER_FLAG "-fsanitize=address" )
		if ( WITH_UBSAN )
			set( SANITIZER_FLAG "${SANITIZER_FLAG},undefined" )
		endif( WITH_UBSAN )
	elseif( WITH_MSAN )
		set( SANITIZER_FLAG "-fsanitize=memory" )
	elseif( WITH_UBSAN )
		set( SANITIZER_FLAG "-fsanitize=undefined" )
	endif( WITH_ASAN )

	set( CMAKE_CXX_FLAGS "${SANITIZER_FLAG} ${CMAKE_CXX_FLAGS}" )
	set( CMAKE_C_FLAGS "${SANITIZER_FLAG} ${CMAKE_C_FLAGS}" )
	set( CMAKE_EXE_LINKER_FLAGS "${SANITIZER_FLAG} ${CMAKE_EXE_LINKER_FLAGS}" )

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.5")
		set( CMAKE_C_FLAGS "-Wno-unused-result ${CMAKE_C_FLAGS}" )
		set( CMAKE_CXX_FLAGS "-Wno-unused-result ${CMAKE_CXX_FLAGS}" )
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		if( APPLE OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "3.6" )
			set( CMAKE_CXX_FLAGS "-Wno-inconsistent-missing-override ${CMAKE_CXX_FLAGS}" )
		endif()
	endif()
	set( CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-missing-field-initializers -ffp-contract=off ${CMAKE_C_FLAGS}" )
	set( CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-missing-field-initializers -ffp-contract=off ${CMAKE_CXX_FLAGS}" )

	# ARM processors (Raspberry Pi, et al) - enable ARM NEON support.
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		set (USE_ARMV8 0 CACHE BOOL "Use ARMv8 instructions - Raspberry Pi 3")
		if (USE_ARMV8)
			set( CMAKE_CXX_FLAGS "-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mtune=cortex-a53 -mhard-float -DNO_SSE ${CMAKE_CXX_FLAGS}" )
		else ()
			set( CMAKE_CXX_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mtune=cortex-a7 -mhard-float -DNO_SSE ${CMAKE_CXX_FLAGS}" )
		endif ()
	endif ()

	if( NOT X64 AND NOT CAN_DO_MFPMATH )
		set( CMAKE_C_FLAGS "-DNO_SSE ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_CXX_FLAGS "-DNO_SSE ${CMAKE_CXX_FLAGS}" )
	endif()

	# Remove extra warnings when using the official DirectX headers.
	# Also, TDM-GCC 4.4.0 no longer accepts glibc-style printf formats as valid,
	# which is a royal pain. The previous version I had been using was fine with them.
	# MinGW: switch to the Windows Unicode API.
	if( WIN32 )
		set( CMAKE_CXX_FLAGS "-Wno-unknown-pragmas -Wno-comment -Wno-format ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_CXX_FLAGS "-D_UNICODE -DUNICODE ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_CXX_FLAGS "-D_WIN32_WINNT=0x0600 ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode" )
	endif()

	# Detect FreeBSD and add flags
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
	endif()

	if( NOT NO_STRIP )
		set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s" )
		set (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s" )
	endif()
endif()

# Check for thread_local keyword, it's optional at the moment

#CHECK_CXX_SOURCE_COMPILES("thread_local int i; int main() { i = 0; }"
#	HAVE_THREAD_LOCAL)

#if( NOT HAVE_THREAD_LOCAL )
#	message( SEND_ERROR "C++ compiler doesn't support thread_local storage duration specifier" )
#endif()

# Check for functions that may or may not exist.

CHECK_FUNCTION_EXISTS( filelength FILELENGTH_EXISTS )
if( FILELENGTH_EXISTS )
	add_definitions( -DHAVE_FILELENGTH=1 )
endif()

CHECK_FUNCTION_EXISTS( strupr STRUPR_EXISTS )
if( NOT STRUPR_EXISTS )
	add_definitions( -DNEED_STRUPR=1 )
endif()

CHECK_FUNCTION_EXISTS( stricmp STRICMP_EXISTS )
if( NOT STRICMP_EXISTS )
	add_definitions( -Dstricmp=strcasecmp )
endif()

CHECK_FUNCTION_EXISTS( strnicmp STRNICMP_EXISTS )
if( NOT STRNICMP_EXISTS )
	add_definitions( -Dstrnicmp=strncasecmp )
endif()

if( NOT MSVC )
	add_definitions( -D__forceinline=inline )
endif()

if( UNIX )
	CHECK_LIBRARY_EXISTS( rt clock_gettime "" CLOCK_GETTIME_IN_RT )
	if( NOT CLOCK_GETTIME_IN_RT )
		CHECK_FUNCTION_EXISTS( clock_gettime CLOCK_GETTIME_EXISTS )
		if( NOT CLOCK_GETTIME_EXISTS )
			message( STATUS "Could not find clock_gettime. Timing statistics will not be available." )
			add_definitions( -DNO_CLOCK_GETTIME )
		endif()
	else()
		set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} rt )
	endif()
endif()

# Flags

# Update gitinfo.h

add_custom_target( revision_check ALL
	COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/tools/updaterevision/UpdateRevision.cmake" source/gitinfo.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# required libraries

set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} "${ZLIB_LIBRARIES}" "${JPEG_LIBRARIES}" "${BZIP2_LIBRARIES}" "${CMAKE_DL_LIBS}" )
if (HAVE_VULKAN)
	set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} "glslang" "SPIRV" "OGLCompiler")
endif()

# ZMUSIC

if( MSVC )
	find_package( ZMusic )
else()
	find_package( ZMusic )
endif()

if( MSVC AND NOT ZMUSIC_FOUND )
	# Use prebuilt library
	set( ZMUSIC_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../bin/windows/zmusic" )
	set( ZMUSIC_INCLUDE_DIR ${ZMUSIC_ROOT_PATH}/include )
	set( ZMUSIC_LIBRARIES zmusiclite )
	if( X64 )
		link_directories( ${ZMUSIC_ROOT_PATH}/64bit )
	else()
		link_directories( ${ZMUSIC_ROOT_PATH}/32bit )
	endif()
	set( ZMUSIC_FOUND TRUE )
endif()

# VPX

if( MSVC AND NOT VPX_FOUND )
	# Use prebuilt library
	set( VPX_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../platform/Windows" )
	set( VPX_INCLUDE_DIR ${VPX_ROOT_PATH}/include )
	set( VPX_LIBRARIES libvpx libcompat-to-msvc )
	if( X64 )
		link_directories( ${VPX_ROOT_PATH}/lib/64 )
	else()
		link_directories( ${VPX_ROOT_PATH}/lib/32 )
		# Workaround for "error LNK2026: module unsafe for SAFESEH image."
		set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO" )
	endif()
	set( VPX_FOUND TRUE )
endif()

if( VPX_FOUND )
	add_definitions( "-DUSE_LIBVPX=1" )
	include_directories( "${VPX_INCLUDE_DIR}" )
	set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${VPX_LIBRARIES} )
endif()


include_directories( "${ZLIB_INCLUDE_DIR}" "${ZMUSIC_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${GDTOA_INCLUDE_DIR}" )

if (WIN32)
	include_directories( "platform/win32" )
endif()


if( ${HAVE_VM_JIT} )
	add_definitions( -DHAVE_VM_JIT )
	include_directories( "${ASMJIT_INCLUDE_DIR}" )
	set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} "${ASMJIT_LIBRARIES}")
endif()


set( PLAT_POSIX_SOURCES
	platform/posix/i_steam.cpp )
set( PLAT_SDL_SOURCES
	platform/posix/sdl/crashcatcher.c
	platform/posix/sdl/hardware.cpp
	platform/posix/sdl/i_gui.cpp
	platform/posix/sdl/i_input.cpp
	platform/posix/sdl/i_joystick.cpp
	platform/posix/sdl/i_main.cpp
	platform/posix/sdl/i_system.cpp
	platform/posix/sdl/sdlglvideo.cpp
	platform/posix/sdl/st_start.cpp )
set( PLAT_UNIX_SOURCES
	platform/posix/unix/i_specialpaths.cpp
	platform/posix/unix/gtk_dialogs.cpp )
set( PLAT_OSX_SOURCES
	platform/posix/osx/iwadpicker_cocoa.mm
	platform/posix/osx/i_specialpaths.mm
	platform/posix/osx/raze.icns )
set( PLAT_COCOA_SOURCES
	platform/posix/cocoa/i_input.mm
	platform/posix/cocoa/i_joystick.cpp
	platform/posix/cocoa/i_main.mm
	platform/posix/cocoa/i_system.mm
	platform/posix/cocoa/i_video.mm
	platform/posix/cocoa/st_console.mm
	platform/posix/cocoa/st_start.mm )

# Start defining source files
set( PLAT_WIN32_SOURCES
	platform/win32/base_sysfb.cpp
	platform/win32/gl_sysfb.cpp
	platform/win32/hardware.cpp
	platform/win32/i_crash.cpp
	platform/win32/i_dijoy.cpp
	platform/win32/i_input.cpp
	platform/win32/i_keyboard.cpp
	platform/win32/i_main.cpp
	platform/win32/i_mouse.cpp
	platform/win32/i_rawps2.cpp
	platform/win32/i_specialpaths.cpp
	platform/win32/i_system.cpp
	platform/win32/i_xinput.cpp
	platform/win32/st_start.cpp
	platform/win32/st_start_util.cpp
	platform/win32/win32basevideo.cpp
	platform/win32/win32glvideo.cpp
	platform/win32/win32vulkanvideo.cpp
	)


if( WIN32 )
	set( SYSTEM_SOURCES_DIR platform/win32 )
	set( SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} )
	set( OTHER_SYSTEM_SOURCES ${PLAT_POSIX_SOURCES} ${PLAT_SDL_SOURCES} ${PLAT_OSX_SOURCES} ${PLAT_COCOA_SOURCES} ${PLAT_UNIX_SOURCES} )

	set( SYSTEM_SOURCES ${SYSTEM_SOURCES} platform/win32/gameres.rc )
elseif( APPLE )
	if( OSX_COCOA_BACKEND )
		set( SYSTEM_SOURCES_DIR platform/posix/cocoa )
		set( SYSTEM_SOURCES ${PLAT_COCOA_SOURCES} )
		set( OTHER_SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} ${PLAT_SDL_SOURCES} ${PLAT_UNIX_SOURCES} )
	else()
		set( SYSTEM_SOURCES_DIR platform/posix/sdl )
		set( SYSTEM_SOURCES ${PLAT_SDL_SOURCES} )
		set( PLAT_OSX_SOURCES ${PLAT_OSX_SOURCES} platform/posix/sdl/i_system.mm )
		set( OTHER_SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} ${PLAT_COCOA_SOURCES} ${PLAT_UNIX_SOURCES} )
	endif()

	set( SYSTEM_SOURCES ${SYSTEM_SOURCES} ${PLAT_POSIX_SOURCES} ${PLAT_OSX_SOURCES} )

	set_source_files_properties( platform/posix/osx/${PROJECT_NAME}.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
	set_source_files_properties( platform/posix/osx/iwadpicker_cocoa.mm PROPERTIES COMPILE_FLAGS -fobjc-exceptions )
else()
	set( SYSTEM_SOURCES_DIR platform/posix/sdl )
	set( SYSTEM_SOURCES ${PLAT_POSIX_SOURCES} ${PLAT_SDL_SOURCES} ${PLAT_UNIX_SOURCES} )
	set( OTHER_SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} ${PLAT_OSX_SOURCES} ${PLAT_COCOA_SOURCES} )
endif()


if( HAVE_PARALLEL_FOR )
	add_definitions( -DHAVE_PARALLEL_FOR=1 )
elseif( HAVE_DISPATCH_APPLY )
	add_definitions( -DHAVE_DISPATCH_APPLY=1 )
else()
	option( NO_OPENMP "Disable usage of OpenMP" OFF )

	if( NOT NO_OPENMP )
		include( FindOpenMP )

		if( OPENMP_FOUND )
			set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
			set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
		endif( OPENMP_FOUND )
	endif( NOT NO_OPENMP )
endif()

add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h
	COMMAND re2c --no-generation-date -s -o ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h ${CMAKE_CURRENT_SOURCE_DIR}/common/engine/sc_man_scanner.re
	DEPENDS re2c ${CMAKE_CURRENT_SOURCE_DIR}/common/engine/sc_man_scanner.re )


include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

#option( SEND_ANON_STATS "Enable sending of anonymous hardware statistics" ON )

#if( NOT SEND_ANON_STATS )
#	add_definitions( -DNO_SEND_STATS )
#endif()

# Project files should be aware of the header files. We can GLOB these since
# there's generally a new cpp for every header so this file will get changed
file( GLOB HEADER_FILES
	build/include/*.h
	glbackend/*.h
	libsmackerdec/include/*.h
	libxmp-lite/include/libxmp-lite/*.h
	libxmp-lite/src/*.h
	core/*.h
	core/2d/*.h
	core/fonts/*.h
	core/utility/*.h
	core/utility/rapidjson/*.h
	core/console/*.h
	core/music/*.h
	core/sound/*.h
	core/sound/backend/*.h
	core/menu/*.h
	core/input/*.h
	core/rendering/*.h
	core/rendering/gl_load/*.h
	core/rendering/gl/system/*.h
	core/rendering/gl/renderer/*.h
	core/rendering/gl/shaders/*.h
	core/rendering/hwrenderer/data/*.h
	core/rendering/hwrenderer/postprocessing/*.h
	core/rendering/hwrenderer/utility/*.h

	common/utility/*.h
	common/engine/*.h
	common/filesystem/*.h
	common/textures/*.h
	common/thirdparty/*.h
	common/thirdparty/rapidjson/*.h
	common/thirdparty/math./*h

	build/src/*.h
	platform/win32/*.h
	platform/posix/*.h
	thirdparty/include/*.h
	thirdparty/include/*.hpp
	thirdparty/imgui/*.h
	core/textures/*.h
	core/textures/formats/*.h
)


# These files will be flagged as "headers" so that they appear in project files
# without being compiled.
set( NOT_COMPILED_SOURCE_FILES
	${OTHER_SYSTEM_SOURCES}
	sc_man_scanner.h
	common/engine/sc_man_scanner.re
	platform/win32/zutil.natvis
)


# Enable fast math for some sources where performance matters (or where the PCH must not be used.) (This would be good for rendering code, but unfortunately that is hopelessly intermingled with the playsim code in engine.cpp.)
set( FASTMATH_SOURCES

	# This should better be made a library subproject later, once things are working.
	libsmackerdec/src/BitReader.cpp
	libsmackerdec/src/FileStream.cpp
	libsmackerdec/src/HuffmanVLC.cpp
	libsmackerdec/src/LogError.cpp
	libsmackerdec/src/SmackerDecoder.cpp

	common/utility/matrix.cpp

	# The rest is only here because it is C, not C++
	core/rendering/gl_load/gl_load.c

	gitinfo.cpp
)


set (PCH_SOURCES

	glbackend/gl_hwtexture.cpp
	glbackend/gl_samplers.cpp
	glbackend/gl_shader.cpp
	glbackend/glbackend.cpp
	glbackend/gl_palmanager.cpp
	glbackend/gl_texture.cpp
	glbackend/hw_draw2d.cpp

	thirdparty/src/base64.cpp
	thirdparty/src/fix16.cpp
	thirdparty/src/md4.cpp
	thirdparty/src/enet.cpp

	# Todo: Split out the license-safe code from this.
	build/src/animvpx.cpp
	build/src/clip.cpp
	build/src/common.cpp
	build/src/compat.cpp
	build/src/defs.cpp
	build/src/engine.cpp
	build/src/hash.cpp
	build/src/hightile.cpp
	build/src/mdsprite.cpp
	build/src/mhk.cpp
	build/src/palette.cpp
	build/src/polymost.cpp
	build/src/pragmas.cpp
	build/src/scriptfile.cpp
	build/src/timer.cpp
	build/src/voxmodel.cpp

	core/animlib.cpp
	core/rts.cpp
	core/gameconfigfile.cpp
	core/gamecvars.cpp
	core/gamecontrol.cpp
	core/inputstate.cpp
	core/searchpaths.cpp
	core/initfs.cpp
	core/statistics.cpp
	core/secrets.cpp
	core/compositesavegame.cpp
	core/savegamehelp.cpp
	core/quotes.cpp
	core/screenshot.cpp
	core/serializer.cpp
	core/raze_music.cpp

	core/2d/v_2ddrawer.cpp
	core/2d/v_draw.cpp
	core/2d/v_drawtext.cpp
	core/2d/renderstyle.cpp
	core/2d/screentext.cpp

	core/fonts/font.cpp
	core/fonts/hexfont.cpp
	core/fonts/singlelumpfont.cpp
	core/fonts/v_font.cpp
	core/fonts/v_text.cpp
	core/fonts/fontchars.cpp

	core/console/c_console.cpp
	core/console/d_event.cpp

	common/thirdparty/sfmt/SFMT.cpp
	common/textures/animtexture.cpp
	common/textures/bitmap.cpp
	common/textures/m_png.cpp
	common/console/c_commandline.cpp
	common/console/c_buttons.cpp
	common/console/c_bind.cpp
	common/console/c_enginecmds.cpp
	common/console/c_consolebuffer.cpp
	common/console/c_cvars.cpp
	common/console/c_dispatch.cpp
	common/console/c_expr.cpp
	common/utility/engineerrors.cpp
	common/utility/i_module.cpp
	common/utility/m_alloc.cpp
	common/utility/utf8.cpp
	common/utility/palette.cpp
	common/utility/files.cpp
	common/utility/files_decompress.cpp
	common/utility/memarena.cpp
	common/utility/cmdlib.cpp
	common/utility/configfile.cpp
	common/utility/i_time.cpp
	common/utility/m_argv.cpp
	common/utility/s_playlist.cpp
	common/utility/zstrformat.cpp
	common/utility/name.cpp
	common/thirdparty/md5.cpp
 	common/thirdparty/superfasthash.cpp
	common/filesystem/filesystem.cpp
	common/filesystem/ancientzip.cpp
	common/filesystem/file_7z.cpp
	common/filesystem/file_grp.cpp
	common/filesystem/file_lump.cpp
	common/filesystem/file_rff.cpp
	common/filesystem/file_wad.cpp
	common/filesystem/file_zip.cpp
	common/filesystem/file_pak.cpp
	common/filesystem/file_whres.cpp
	common/filesystem/file_directory.cpp
	common/filesystem/resourcefile.cpp
	common/engine/sc_man.cpp
	common/engine/palettecontainer.cpp
	common/engine/stringtable.cpp

	core/utility/stats.cpp
	
	core/textures/buildtiles.cpp
	core/textures/texture.cpp
	core/textures/image.cpp
	core/textures/imagetexture.cpp
	core/textures/imagehelpers.cpp
	core/textures/formats/ddstexture.cpp
	core/textures/formats/jpegtexture.cpp
	core/textures/formats/pcxtexture.cpp
	core/textures/formats/pngtexture.cpp
	core/textures/formats/tgatexture.cpp
	core/textures/formats/stbtexture.cpp
	core/textures/formats/arttexture.cpp

	core/music/music.cpp
	core/music/i_music.cpp
	core/music/i_soundfont.cpp
	core/music/music_config.cpp
	core/music/music_midi_base.cpp
	core/music/s_advsound.cpp

	core/sound/backend/oalsound.cpp
	core/sound/backend/i_sound.cpp
	core/sound/s_sound.cpp
	core/sound/s_environment.cpp

	core/menu/imagescroller.cpp
	core/menu/joystickmenu.cpp
	core/menu/listmenu.cpp
	core/menu/savegamemanager.cpp
	core/menu/loadsavemenu.cpp
	core/menu/menu.cpp
	core/menu/menudef.cpp
	core/menu/menuinput.cpp
	core/menu/messagebox.cpp
	core/menu/optionmenu.cpp
	core/menu/resolutionmenu.cpp

	#core/input/i_joystick.cpp
	#core/input/i_input.cpp
	core/input/m_joy.cpp

	core/rendering/r_videoscale.cpp
	core/rendering/v_framebuffer.cpp
	core/rendering/v_video.cpp
	core/rendering/gl/renderer/gl_renderer.cpp
	core/rendering/gl/renderer/gl_postprocess.cpp
	core/rendering/gl/renderer/gl_postprocessstate.cpp
	core/rendering/gl/renderer/gl_renderbuffers.cpp
	core/rendering/gl/shaders/gl_shaderprogram.cpp
	core/rendering/gl/system/gl_buffers.cpp
	core/rendering/gl/system/gl_debug.cpp
	core/rendering/gl/system/gl_framebuffer.cpp
	core/rendering/gl_load/gl_interface.cpp
	core/rendering/hwrenderer/data/flatvertices.cpp
	core/rendering/hwrenderer/postprocessing/hw_postprocess.cpp
	core/rendering/hwrenderer/postprocessing/hw_postprocess_cvars.cpp
	core/rendering/hwrenderer/utility/hw_shaderpatcher.cpp
)

macro( use_precompiled_header )
	if( MSVC )
		enable_precompiled_headers( "${ARGV0}/g_pch.h" PCH_SOURCES )
		# The original Build code was written with unsigned chars and unfortunately they still haven't been eliminated entirely.
		# All other code should stay with signed chars. What a mess... :(
	else()
		# Temporary solution for compilers other than MSVC
		set_source_files_properties( ${PCH_SOURCES} PROPERTIES COMPILE_FLAGS "-include g_pch.h" )
	endif()
endmacro()

use_precompiled_header(".")

add_executable( ${PROJECT_NAME} WIN32 MACOSX_BUNDLE
	${HEADER_FILES}
	${NOT_COMPILED_SOURCE_FILES}
	#__autostart.cpp
	${SYSTEM_SOURCES}
	${FASTMATH_SOURCES}
	${PCH_SOURCES}
	common/utility/x86.cpp
	common/thirdparty/strnatcmp.c
	common/utility/zstring.cpp
	common/utility/findfile.cpp
	common/thirdparty/math/asin.c
	common/thirdparty/math/atan.c
	common/thirdparty/math/const.c
	common/thirdparty/math/cosh.c
	common/thirdparty/math/exp.c
	common/thirdparty/math/isnan.c
	common/thirdparty/math/log.c
	common/thirdparty/math/log10.c
	common/thirdparty/math/mtherr.c
	common/thirdparty/math/polevl.c
	common/thirdparty/math/pow.c
	common/thirdparty/math/powi.c
	common/thirdparty/math/sin.c
	common/thirdparty/math/sinh.c
	common/thirdparty/math/sqrt.c
	common/thirdparty/math/tan.c
	common/thirdparty/math/tanh.c
	common/thirdparty/math/fastsin.cpp
)

#set_source_files_properties( ${FASTMATH_SOURCES} PROPERTIES COMPILE_FLAGS ${DEM_FASTMATH_FLAG} )
set_source_files_properties( xlat/parse_xlat.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xlat_parser.c" )
set_source_files_properties( common/engine/sc_man.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h" )
set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )


if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
	# [BL] Solaris requires these to be explicitly linked.
	set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} nsl socket)
endif()

if( UNIX )
	find_package( Backtrace )
	if(Backtrace_FOUND)
		set( PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${Backtrace_LIBRARIES} )
	endif()
endif()

target_link_libraries( ${PROJECT_NAME} ${PROJECT_LIBRARIES} ${ZMUSIC_LIBRARIES} gdtoa lzma duke3d blood rr sw exhumed )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
    build/include
	libxmp-lite/include
	libxmp-lite/include/libxmp-lite
	libsmackerdec/include
	thirdparty/include
	thirdparty/imgui
	core
	core/2d
	core/fonts
	core/utility
	core/console
	core/textures
	core/music
	core/sound
	core/sound/backend
	core/dobject
	core/menu
	core/input
	core/rendering/gl_load
	core/rendering/gl/system
	core/rendering/gl/renderer
	core/rendering/gl/shaders
	core/rendering/hwrenderer/data
	core/rendering/hwrenderer/postprocessing
	core/rendering/hwrenderer/utility
	core/rendering
	platform
	common/thirdparty
	common/textures
	common/filesystem
	common/utility
	common/console
	common/engine
	${CMAKE_BINARY_DIR}/libraries/gdtoa

	${SYSTEM_SOURCES_DIR}
)

if (NOT WIN32)
	include_directories(platform/posix)
endif()

add_dependencies( ${PROJECT_NAME} revision_check )

# Due to some quirks, we need to do this in this order
if( NOT COMPILE_OUTPUT_OLDSTYLE )
	# RUNTIME_OUTPUT_DIRECTORY does not exist in CMake 2.4.
	# Linux distributions are slow to adopt 2.6. :(
	set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_DIECTRORY} )
	set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME} )
else()
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		RUNTIME_OUTPUT_NAME ${PROJECT_NAME}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${COMPILE_OUTPUT_DIECTRORY}
		RUNTIME_OUTPUT_NAME_DEBUG ${PROJECT_NAME}d
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${COMPILE_OUTPUT_DIECTRORY}
		RUNTIME_OUTPUT_NAME_MINSIZEREL ${PROJECT_NAME}msr
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${COMPILE_OUTPUT_DIECTRORY}
		RUNTIME_OUTPUT_NAME_RELWITHDEBINFO ${PROJECT_NAME}rd
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${COMPILE_OUTPUT_DIECTRORY}
	)
endif()

if( MSVC )
	option( COMPILE_GENERATE_MAPFILE "Generate .map file for debugging." OFF )
	set( LINKERSTUFF "/MANIFEST:NO" )

	if( COMPILE_GENERATE_MAPFILE )
		set( LINKERSTUFF "${LINKERSTUFF} /MAP" )
	endif()
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${LINKERSTUFF})

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\platform\\win32\\manifest.game.xml\" -outputresource:\"$<TARGET_FILE:${PROJECT_NAME}>\"\;\#1
		COMMENT "Adding manifest..."
	)

endif()

if( NOT WIN32 AND NOT APPLE )
	FILE( WRITE ${CMAKE_CURRENT_BINARY_DIR}/link-make "if [ ! -e ${COMPILE_OUTPUT_DIECTRORY}/${PROJECT_NAME} ]; then ln -sf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} ${COMPILE_OUTPUT_DIECTRORY}/${PROJECT_NAME}; fi" )
	add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/link-make
		COMMAND /bin/sh -c ${CMAKE_CURRENT_BINARY_DIR}/link-make )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${CMAKE_SOURCE_DIR}/soundfont/${PROJECT_NAME}.sf2 $<TARGET_FILE_DIR:${PROJECT_NAME}>/soundfonts/${PROJECT_NAME}.sf2
)

if( CMAKE_COMPILER_IS_GNUCXX )
	# GCC misoptimizes this file
endif()

if( APPLE )
	set( LINK_FRAMEWORKS "-framework Cocoa -framework IOKit -framework OpenGL")

	if( HAVE_VULKAN )
		set( LINK_FRAMEWORKS "${LINK_FRAMEWORKS} -framework QuartzCore" )
	endif()

	set_target_properties(${PROJECT_NAME} PROPERTIES
		LINK_FLAGS "${LINK_FRAMEWORKS}"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/osx/${PROJECT_NAME}-info.plist"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "" )

	# Dymanic libraries like libvulkan.dylib or libMoltenVK.dylib will be loaded by dlopen()
	# if placed in the directory with the main executable
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rpath @executable_path" )
endif()

if( WIN32 )
	set( INSTALL_PATH . CACHE STRING "Directory where the executable will be placed during install." )
else()
	set( INSTALL_PATH bin CACHE STRING "Directory where the executable will be placed during install." )
endif()
install(TARGETS ${PROJECT_NAME}
		DESTINATION ${INSTALL_PATH}
		COMPONENT "Game executable")

source_group("Utility" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/utility/.+")
source_group("Core" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/.+")
source_group("Core\\Textures" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/textures/.+")
source_group("Core\\Textures\\Formats" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/textures/formats/.+")
source_group("Core\\Utility" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/utility/.+")
source_group("Core\\2D" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/2d/.+")
source_group("Core\\Console" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/console/.+")
source_group("Core\\Fonts" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/fonts/.+")
source_group("Core\\Music" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/music/.+")
source_group("Core\\Sound" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/sound/.+")
source_group("Core\\Sound\\Backend" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/sound/backend/.+")
source_group("Core\\DObject" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/dobject/.+")
source_group("Core\\Menu" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/menu/.+")
source_group("Core\\Rendering" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/rendering/.+")
source_group("Core\\Rendering\\GL_Load" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/rendering/gl_load/.+")
source_group("Core\\Rendering\\GL\\System" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/core/rendering/gl/system.+")
source_group("Platform" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/platform/.+")
source_group("Platform\\Win32" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/platform/win32/.+")
source_group("Platform\\POSIX" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/.+")
source_group("Common" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/.+")
source_group("Common\\Console" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/console/.+")
source_group("Common\\Utility" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/utility/.+")
source_group("Common\\Engine" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/engine/.+")
source_group("Common\\File System" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/filesystem/.+")
source_group("Common\\Textures" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/textures/.+")
source_group("Common\\Third Party" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/thirdparty/.+")
source_group("Common\\Third Party\\Math" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/thirdparty/math/.+")
source_group("Common\\Third Party\\RapidJSON" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/thirdparty/rapidjson/.+")
source_group("Common\\Third Party\\SFMT" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/thirdparty/sfmt/.+")
source_group("Utility\\Smackerdec" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/smackerdec/.+")
source_group("Utility\\Smackerdec\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libsmackerdec/include/.+")
source_group("Utility\\Smackerdec\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libsmackerdec/src/.+")
source_group("Utility\\XMP-Lite" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/.+")
source_group("Utility\\XMP-Lite Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/include/libxmp-lite/.+")
source_group("Utility\\XMP-Lite Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/src/.+")
source_group("Utility\\Third Party" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/.+")
source_group("Utility\\Third Party Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include/.+")
source_group("Utility\\Third Party Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/src/.+")
source_group("Utility\\RapidJSON" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/utility/rapidjson/.+")
source_group("OpenGL Backend" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glbackend/.+")
source_group("Build Engine" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/.+")
source_group("Build Engine\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/include/.+")
source_group("Build Engine\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/src/.+")

macro( add_game_library )
	file( GLOB HEADER_FILES src/*.h )
	if( NOT_COMPILED_SOURCE_FILES )
		set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )
	endif()
	use_precompiled_header("..")
	add_library( ${ARGV0} STATIC
		${PCH_SOURCES}
		${HEADER_FILES}
		${NOT_COMPILED_SOURCE_FILES}
	)
endmacro()

require_stricmp()
require_strnicmp()

unset( PCH_SOURCES )
unset( HEADER_FILES )
unset( NOT_COMPILED_SOURCE_FILES )

add_subdirectory( duke3d )
add_subdirectory( blood )
add_subdirectory( rr )
add_subdirectory( sw )
add_subdirectory( exhumed )
