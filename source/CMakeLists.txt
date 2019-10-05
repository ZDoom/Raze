cmake_minimum_required( VERSION 2.8.7 )

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

option( DYN_FLUIDSYNTH "Dynamically load fluidsynth" ON )
option( DYN_OPENAL "Dynamically load OpenAL" ON )
option( DYN_SNDFILE "Dynamically load libsndfile" ON )
option( DYN_MPG123 "Dynamically load libmpg123" ON )

if( APPLE )
    option( OSX_COCOA_BACKEND "Use native Cocoa backend instead of SDL" ON )
endif()

if( ${DEMOLITION_TARGET_ARCH} MATCHES "x86_64" )
	set( X64 64 )
endif()

if( X64 OR ${DEMOLITION_TARGET_ARCH} MATCHES "i386" )
	add_definitions( -DARCH_IA32 )
endif()

if( NOT DEMOLITION_LIBS )
	set( DEMOLITION_LIBS "" )
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


	if( ( MSVC14 AND NOT CMAKE_GENERATOR_TOOLSET STREQUAL "v140_xp" ) OR	# For VS 2015.
		( MSVC15 AND NOT CMAKE_GENERATOR_TOOLSET STREQUAL "v141_xp" ) )		# For VS 2017.
		# for modern Windows SDKs the DirectX headers should be available by default.
		set( DX_dinput8_LIBRARY dinput8 )
	else()

		find_path( XINPUT_INCLUDE_DIR xinput.h
			PATHS ENV DXSDK_DIR
			PATH_SUFFIXES Include )
		if( NOT XINPUT_INCLUDE_DIR )
			# Modern versions of the Windows SDK include xinput.h. Unfortunately,
			# CMake cannot find this file via find_path, so we check for it using
			# CHECK_INCLUDE_FILES. windows.h must be included before xinput.h.
			CHECK_INCLUDE_FILES( "windows.h;xinput.h" XINPUT_H_FOUND )
			if( NOT XINPUT_H_FOUND )
				message( WARNING "Could not find xinput.h. XInput will be disabled." )
				add_definitions( -DNO_XINPUT )
			endif()
		else()
			include_directories( ${XINPUT_INCLUDE_DIR} )
		endif()

		find_library( DX_dinput8_LIBRARY dinput8
			PATHS ENV DXSDK_DIR
			PATH_SUFFIXES Lib Lib/${XBITS} )
		if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
			find_library( DX_dxguid_LIBRARY dxguid
				PATHS ENV DXSDK_DIR
				PATH_SUFFIXES Lib Lib/${XBITS} )
		endif()

		# Modern versions of the Windows SDK include dinput8. Unfortunately,
		# CMake cannot find these libraries via find_library.
		if( NOT DX_dinput8_LIBRARY )
			# If we got this far, assume dinput8 is in the system library path.
			set( DX_dinput8_LIBRARY dinput8 )
		endif()

		if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
			# Modern versions of the Windows SDK do NOT include dxguid. Its contents
			# were moved to dinput8.
			if( NOT DX_dxguid_LIBRARY )
				message( STATUS "Could not find dxguid. Build may fail on old Windows SDKs.")
			endif()
		endif()
	endif()

	if( X64 ) 
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/Windows/lib/64)
	else()
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/Windows/lib/32)
	endif()
	
	set( DEMOLITION_LIBS
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

		libSDL2main
		libSDL2

		# this local library crap needs to go away. The Ogg/Vorbis libraries are easy to replace with libsndfile but there doesn't seem to be anything to get a working libvpx that doesn't force linking with MinGW dependencies.
		libogg
		libvorbis
		libvorbisfile
		libvpx
		libcompat-to-msvc
		
		)
		

		

	if( NOT DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
		set( DEMOLITION_LIBS ${DEMOLITION_LIBS} DelayImp )
	endif()

	if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
		if( DX_dxguid_LIBRARY )
			list( APPEND DEMOLITION_LIBS "${DX_dxguid_LIBRARY}" )
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
					set( DEMOLITION_LIBS ${DEMOLITION_LIBS} ${GTK3_LIBRARIES} )
				endif()
				include_directories( ${GTK3_INCLUDE_DIRS} )
				link_directories( ${GTK3_LIBRARY_DIRS} )
			else()
				pkg_check_modules( GTK2 gtk+-2.0 )
				if( GTK2_FOUND )
					if( NOT DYN_GTK )
						set( DEMOLITION_LIBS ${DEMOLITION_LIBS} ${GTK2_LIBRARIES} )
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
		set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${SDL2_LIBRARY}" )
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
				set( DEMOLITION_LIBS ${OPENAL_LIBRARY} ${DEMOLITION_LIBS} )
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

	set(MPG123_FOUND NO)
	set(SNDFILE_FOUND NO)
else()
	# Search for libSndFile

	if ( NOT DYN_SNDFILE )
		find_package( SndFile )
	endif()

	# Search for libmpg123

	if ( NOT DYN_MPG123 )
		find_package( MPG123 )
	endif()
endif()

# Search for FluidSynth

find_package( FluidSynth )

# Decide on SSE setup

set( SSE_MATTERS NO )

# with global use of SSE 2 we do not need special handling for selected files
if (NOT DEMOLITION_USE_SSE2)
	# SSE only matters on 32-bit targets. We check compiler flags to know if we can do it.
	if( CMAKE_SIZEOF_VOID_P MATCHES "4" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES ppc )
		CHECK_CXX_COMPILER_FLAG( "-msse2 -mfpmath=sse" CAN_DO_MFPMATH )
		CHECK_CXX_COMPILER_FLAG( -arch:SSE2 CAN_DO_ARCHSSE2 )
		if( CAN_DO_MFPMATH )
			set( SSE1_ENABLE "-msse -mfpmath=sse" )
			set( SSE2_ENABLE "-msse2 -mfpmath=sse" )
			set( SSE_MATTERS YES )
		elseif( CAN_DO_ARCHSSE2 )
			set( SSE1_ENABLE -arch:SSE )
			set( SSE2_ENABLE -arch:SSE2 )
			set( SSE_MATTERS YES )
		endif()
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

	set( REL_CXX_FLAGS "-fno-rtti" )
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

	# Use the highest C++ standard available since VS2015 compiles with C++14
	# but we only require C++11.  The recommended way to do this in CMake is to
	# probably to use target_compile_features, but I don't feel like maintaining
	# a list of features we use.
	CHECK_CXX_COMPILER_FLAG( "-std=gnu++14" CAN_DO_CPP14 )
	if ( CAN_DO_CPP14 )
		set ( CMAKE_CXX_FLAGS "-std=gnu++14 ${CMAKE_CXX_FLAGS}" )
	else ()
		CHECK_CXX_COMPILER_FLAG( "-std=gnu++1y" CAN_DO_CPP1Y )
		if ( CAN_DO_CPP1Y )
			set ( CMAKE_CXX_FLAGS "-std=gnu++1y ${CMAKE_CXX_FLAGS}" )
		else ()
			CHECK_CXX_COMPILER_FLAG( "-std=gnu++11" CAN_DO_CPP11 )
			if ( CAN_DO_CPP11 )
				set ( CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}" )
			else ()
				CHECK_CXX_COMPILER_FLAG( "-std=gnu++0x" CAN_DO_CPP0X )
				if ( CAN_DO_CPP0X )
					set ( CMAKE_CXX_FLAGS "-std=gnu++0x ${CMAKE_CXX_FLAGS}" )
				endif ()
			endif ()
		endif ()
	endif ()

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

CHECK_CXX_SOURCE_COMPILES("thread_local int i; int main() { i = 0; }"
	HAVE_THREAD_LOCAL)

if( NOT HAVE_THREAD_LOCAL )
	message( SEND_ERROR "C++ compiler doesn't support thread_local storage duration specifier" )
endif()

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
		set( DEMOLITION_LIBS ${DEMOLITION_LIBS} rt )
	endif()
endif()

# Flags

# Update gitinfo.h

add_custom_target( revision_check ALL
	COMMAND updaterevision source/gitinfo.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS updaterevision )

# Libraries Demolition needs

message( STATUS "Fluid synth libs: ${FLUIDSYNTH_LIBRARIES}" )
set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${ZLIB_LIBRARIES}" "${JPEG_LIBRARIES}" "${BZIP2_LIBRARIES}" "${GME_LIBRARIES}" "${CMAKE_DL_LIBS}" )
if (HAVE_VULKAN)
	set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "glslang" "SPIRV" "OGLCompiler")
endif()

# Ugh... These precompiled dependencies need to go.
if (WIN32)
	include_directories( "${ZLIB_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${GME_INCLUDE_DIR}" "${ENET_INCLUDE_DIR}" "${GDTOA_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../platform/windows/include" "${CMAKE_CURRENT_SOURCE_DIR}/../platform/windows/include/vpx" "${CMAKE_CURRENT_SOURCE_DIR}/../platform/windows/include/sdl2")
else ()
include_directories( "${ZLIB_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${GME_INCLUDE_DIR}" "${ENET_INCLUDE_DIR}" "${GDTOA_INCLUDE_DIR}")
endif()


if( ${HAVE_VM_JIT} )
	add_definitions( -DHAVE_VM_JIT )
	include_directories( "${ASMJIT_INCLUDE_DIR}" )
	set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${ASMJIT_LIBRARIES}")
endif()

if( SNDFILE_FOUND )
    set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${SNDFILE_LIBRARIES}" )
    include_directories( "${SNDFILE_INCLUDE_DIRS}" )
endif()
if( MPG123_FOUND )
    set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${MPG123_LIBRARIES}" )
    include_directories( "${MPG123_INCLUDE_DIR}" )
endif()
if( NOT DYN_FLUIDSYNTH )
	if( FLUIDSYNTH_FOUND )
		set( DEMOLITION_LIBS ${DEMOLITION_LIBS} "${FLUIDSYNTH_LIBRARIES}" )
		include_directories( "${FLUIDSYNTH_INCLUDE_DIR}" )
	endif()
endif()



# Start defining source files for Demolition
set( PLAT_WIN32_SOURCES
	audiolib/src/driver_directsound.cpp
	glad/src/glad_wgl.c
	platform/win32/winbits.cpp
	
	duke3d/src/startwin.game.cpp	
	rr/src/startwin.game.cpp	
	blood/src/startwin.game.cpp	
	# This needs a rework anyway in order to consolidate the startup dialogs so don't bother to sort in properly.
	#startgtk.game.cpp
	#startosx.game.mm
	
	)

set( PLAT_POSIX_SOURCES
	audiolib/src/driver_sdl.cpp
	audiolib/src/sdlmusic.cpp
	)
	
set( PLAT_SDL_SOURCES
	)
	
set( PLAT_UNIX_SOURCES
	platform/gtk/dynamicgtk.cpp
	platform/gtk/gtkbits.cpp
	)
	
set( PLAT_OSX_SOURCES
	platform/macos/SDLMain.mm
	platform/macos/osxbits.mm
	)
	
set( PLAT_COCOA_SOURCES
	)
	


if( WIN32 )
	#set( SYSTEM_SOURCES_DIR win32 )
	set( SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} )
	set( OTHER_SYSTEM_SOURCES ${PLAT_POSIX_SOURCES} ${PLAT_SDL_SOURCES} ${PLAT_OSX_SOURCES} ${PLAT_COCOA_SOURCES} ${PLAT_UNIX_SOURCES} )

	set( SYSTEM_SOURCES ${SYSTEM_SOURCES} platform/win32/gameres.rc )
elseif( APPLE )
	if( OSX_COCOA_BACKEND )
		#set( SYSTEM_SOURCES_DIR posix posix/cocoa )
		set( SYSTEM_SOURCES ${PLAT_COCOA_SOURCES} )
		set( OTHER_SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} ${PLAT_SDL_SOURCES} ${PLAT_UNIX_SOURCES} )
	else()
		#set( SYSTEM_SOURCES_DIR posix posix/sdl )
		set( SYSTEM_SOURCES ${PLAT_SDL_SOURCES} )
		##set( PLAT_OSX_SOURCES ${PLAT_OSX_SOURCES} posix/sdl/i_system.mm )
		set( OTHER_SYSTEM_SOURCES ${PLAT_WIN32_SOURCES} ${PLAT_COCOA_SOURCES} ${PLAT_UNIX_SOURCES} )
	endif()

	set( SYSTEM_SOURCES ${SYSTEM_SOURCES} ${PLAT_POSIX_SOURCES} ${PLAT_OSX_SOURCES} )

	##set_source_files_properties( posix/osx/demolition.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
	##set_source_files_properties( posix/osx/iwadpicker_cocoa.mm PROPERTIES COMPILE_FLAGS -fobjc-exceptions )
else()
	#set( SYSTEM_SOURCES_DIR posix posix/sdl )
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

if (FALSE)	# for later
add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h
	COMMAND re2c --no-generation-date -s -o ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h ${CMAKE_CURRENT_SOURCE_DIR}/utility/sc_man_scanner.re
	DEPENDS re2c ${CMAKE_CURRENT_SOURCE_DIR}/utility/sc_man_scanner.re )
	
endif ()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

if( DYN_SNDFILE)
	add_definitions( -DHAVE_SNDFILE -DDYN_SNDFILE )
elseif( SNDFILE_FOUND )
	add_definitions( -DHAVE_SNDFILE )
endif()

if( DYN_MPG123)
	add_definitions( -DHAVE_MPG123 -DDYN_MPG123 )
elseif( MPG123_FOUND )
	add_definitions( -DHAVE_MPG123 )
endif()

if( DYN_FLUIDSYNTH )
	add_definitions( -DHAVE_FLUIDSYNTH -DDYN_FLUIDSYNTH )
elseif( FLUIDSYNTH_FOUND )
	add_definitions( -DHAVE_FLUIDSYNTH )
endif()

#option( SEND_ANON_STATS "Enable sending of anonymous hardware statistics" ON )

#if( NOT SEND_ANON_STATS )
#	add_definitions( -DNO_SEND_STATS )
#endif()


# Disable ADLMIDI's and OPNMIDI's MIDI Sequencer
add_definitions(-DADLMIDI_DISABLE_MIDI_SEQUENCER)
add_definitions(-DOPNMIDI_DISABLE_MIDI_SEQUENCER)

# Disable OPNMIDI's experimental yet emulator (using of it has some issues and missing notes in playback)
add_definitions(-DOPNMIDI_DISABLE_GX_EMULATOR)

# Project files should be aware of the header files. We can GLOB these since
# there's generally a new cpp for every header so this file will get changed
file( GLOB HEADER_FILES
	audiolib/include/*.h
	audiolib/src/*.h
	blood/src/*.h
	build/include/*.h
	glad/include/glad/*.h
	glad/include/Khr/*.h
	glbackend/*.h
	libsmackerdec/include/*.h
	libxmp-lite/include/libxmp-lite/*.h
	libxmp-lite/src/*.h
	mact/include/*.h
	mact/src/*.h
	common/*.h
	common/utility/*.h

	build/src/*.h
	duke3d/src/*.h
	rr/src/*.h
	#sw/src/*.h Shadow Warrior does not work yet.
	thirdparty/include/*.h
	thirdparty/include/*.hpp
	common/textures/*.h
	common/textures/formats/*.h
)


# These files will be flagged as "headers" so that they appear in project files
# without being compiled.
set( NOT_COMPILED_SOURCE_FILES
	${OTHER_SYSTEM_SOURCES}
	build/src/sdlkeytrans.cpp
	duke3d/src/gamestructures.cpp
	rr/src/gamestructures.cpp
		#sc_man_scanner.h
	#utility/sc_man_scanner.re
)


# Enable fast math for some sources where performance matters (or where the PCH must not be used.) (This would be good for rendering code, but unfortunately that is hopelessly intermingled with the playsim code in engine.cpp.)
set( FASTMATH_SOURCES

	# This should better be made a library subproject later, once things are working.
	libsmackerdec/src/BitReader.cpp
	libsmackerdec/src/FileStream.cpp
	libsmackerdec/src/HuffmanVLC.cpp
	libsmackerdec/src/LogError.cpp
	libsmackerdec/src/SmackerDecoder.cpp

	# This will go away once GZDoom's music backend has been plugged in.
	libxmp-lite/src/common.c
	libxmp-lite/src/control.c
	libxmp-lite/src/dataio.c
	libxmp-lite/src/effects.c
	libxmp-lite/src/filter.c
	libxmp-lite/src/format.c
	libxmp-lite/src/hio.c
	libxmp-lite/src/itsex.c
	libxmp-lite/src/it_load.c
	libxmp-lite/src/lfo.c
	libxmp-lite/src/load.c
	libxmp-lite/src/load_helpers.c
	libxmp-lite/src/memio.c
	libxmp-lite/src/mixer.c
	libxmp-lite/src/mix_all.c
	libxmp-lite/src/mod_load.c
	libxmp-lite/src/mtm_load.c
	libxmp-lite/src/period.c
	libxmp-lite/src/player.c
	libxmp-lite/src/read_event.c
	libxmp-lite/src/s3m_load.c
	libxmp-lite/src/sample.c
	libxmp-lite/src/scan.c
	libxmp-lite/src/smix.c
	libxmp-lite/src/virtual.c
	libxmp-lite/src/win32.c
	libxmp-lite/src/xm_load.c

	# The rest is only here because it is C, not C++
	glad/src/glad.c
	
	thirdparty/src/lz4.c
	thirdparty/src/xxhash.c

	# Will we need this anymore with ZLib being available?
	#thirdparty/src/miniz.c
	#thirdparty/src/miniz_tdef.c
	#thirdparty/src/miniz_tinfl.c
	
	# Another bit of cruft just to make S(hit)DL happy...
	sdlappicon.cpp
)


set (PCH_SOURCES
	audiolib/src/drivers.cpp
	audiolib/src/driver_nosound.cpp
	audiolib/src/flac.cpp
	audiolib/src/formats.cpp
	audiolib/src/fx_man.cpp
	audiolib/src/gmtimbre.cpp
	audiolib/src/midi.cpp
	audiolib/src/mix.cpp
	audiolib/src/mixst.cpp
	audiolib/src/mpu401.cpp
	audiolib/src/multivoc.cpp
	audiolib/src/music.cpp
	audiolib/src/pitch.cpp
	audiolib/src/vorbis.cpp
	audiolib/src/xa.cpp
	audiolib/src/xmp.cpp
	
	glbackend/gl_hwtexture.cpp
	glbackend/gl_samplers.cpp
	glbackend/gl_shader.cpp
	glbackend/glbackend.cpp
	
	mact/src/animlib.cpp
	mact/src/control.cpp
	mact/src/joystick.cpp
	mact/src/keyboard.cpp
	mact/src/scriplib.cpp
	
	thirdparty/src/sjson.cpp
	thirdparty/src/crc32.cpp
	thirdparty/src/fix16.cpp
	thirdparty/src/fix16_str.cpp
	thirdparty/src/md4.cpp
	
	# Todo: Split out the license-safe code from this.
	build/src/2d.cpp
	build/src/a-c.cpp
	build/src/animvpx.cpp
	build/src/baselayer.cpp
	build/src/cache1d.cpp
	build/src/clip.cpp
	build/src/colmatch.cpp
	build/src/common.cpp
	build/src/compat.cpp
	build/src/defs.cpp
	build/src/engine.cpp
	build/src/glsurface.cpp
	build/src/hash.cpp
	build/src/hightile.cpp
	build/src/kplib.cpp
	build/src/mdsprite.cpp
	build/src/mhk.cpp
	build/src/mutex.cpp
	build/src/osd.cpp
	build/src/palette.cpp
	build/src/pngwrite.cpp
	build/src/polymost.cpp
	build/src/pragmas.cpp
	build/src/rev.cpp
	build/src/screenshot.cpp
	build/src/scriptfile.cpp
	build/src/sdlayer.cpp
	build/src/smalltextfont.cpp
	build/src/softsurface.cpp
	build/src/texcache.cpp
	build/src/textfont.cpp
	build/src/tiles.cpp
	build/src/timer.cpp
	build/src/voxmodel.cpp
	
	duke3d/src/actors.cpp
	duke3d/src/anim.cpp
	duke3d/src/cheats.cpp
	duke3d/src/cmdline.cpp
	duke3d/src/common.cpp
	duke3d/src/config.cpp
	duke3d/src/demo.cpp
	duke3d/src/game.cpp
	duke3d/src/gamedef.cpp
	duke3d/src/gameexec.cpp
	duke3d/src/gamevars.cpp
	duke3d/src/global.cpp
	duke3d/src/grpscan.cpp
	duke3d/src/input.cpp
	duke3d/src/menus.cpp
	duke3d/src/namesdyn.cpp
	duke3d/src/network.cpp
	duke3d/src/osdcmds.cpp
	duke3d/src/osdfuncs.cpp
	duke3d/src/player.cpp
	duke3d/src/premap.cpp
	duke3d/src/rts.cpp
	duke3d/src/savegame.cpp
	duke3d/src/sbar.cpp
	duke3d/src/screens.cpp
	duke3d/src/screentext.cpp
	duke3d/src/sector.cpp
	duke3d/src/sounds.cpp
	duke3d/src/soundsdyn.cpp
	
	rr/src/actors.cpp
	rr/src/anim.cpp
	rr/src/cheats.cpp
	rr/src/cmdline.cpp
	rr/src/common.cpp
	rr/src/config.cpp
	rr/src/demo.cpp
	rr/src/game.cpp
	rr/src/gamedef.cpp
	rr/src/gameexec.cpp
	rr/src/gamevars.cpp
	rr/src/global.cpp
	rr/src/grpscan.cpp
	rr/src/input.cpp
	rr/src/menus.cpp
	rr/src/namesdyn.cpp
	rr/src/net.cpp
	rr/src/osdcmds.cpp
	rr/src/osdfuncs.cpp
	rr/src/player.cpp
	rr/src/premap.cpp
	rr/src/rts.cpp
	rr/src/savegame.cpp
	rr/src/sbar.cpp
	rr/src/screens.cpp
	rr/src/screentext.cpp
	rr/src/sector.cpp
	rr/src/sounds.cpp
	rr/src/soundsdyn.cpp

	blood/src/actor.cpp
	blood/src/ai.cpp
	blood/src/aibat.cpp
	blood/src/aibeast.cpp
	blood/src/aiboneel.cpp
	blood/src/aiburn.cpp
	blood/src/aicaleb.cpp
	blood/src/aicerber.cpp
	blood/src/aicult.cpp
	blood/src/aigarg.cpp
	blood/src/aighost.cpp
	blood/src/aigilbst.cpp
	blood/src/aihand.cpp
	blood/src/aihound.cpp
	blood/src/aiinnoc.cpp
	blood/src/aipod.cpp
	blood/src/airat.cpp
	blood/src/aispid.cpp
	blood/src/aitchern.cpp
	blood/src/aiunicult.cpp
	blood/src/aizomba.cpp
	blood/src/aizombf.cpp
	blood/src/asound.cpp
	blood/src/barf.cpp
	blood/src/blood.cpp
	blood/src/callback.cpp
	blood/src/choke.cpp
	blood/src/common.cpp
	blood/src/config.cpp
	blood/src/controls.cpp
	blood/src/credits.cpp
	blood/src/db.cpp
	blood/src/demo.cpp
	blood/src/dude.cpp
	blood/src/endgame.cpp
	blood/src/eventq.cpp
	blood/src/fire.cpp
	blood/src/fx.cpp
	blood/src/gamemenu.cpp
	blood/src/gameutil.cpp
	blood/src/getopt.cpp
	blood/src/gib.cpp
	blood/src/globals.cpp
	blood/src/inifile.cpp
	blood/src/iob.cpp
	blood/src/levels.cpp
	blood/src/loadsave.cpp
	blood/src/map2d.cpp
	blood/src/menu.cpp
	blood/src/messages.cpp
	blood/src/mirrors.cpp
	blood/src/misc.cpp
	blood/src/network.cpp
	blood/src/osdcmd.cpp
	blood/src/player.cpp
	blood/src/qav.cpp
	blood/src/qheap.cpp
	blood/src/replace.cpp
	blood/src/resource.cpp
	blood/src/screen.cpp
	blood/src/sectorfx.cpp
	blood/src/seq.cpp
	blood/src/sfx.cpp
	blood/src/sound.cpp
	blood/src/tile.cpp
	blood/src/trig.cpp
	blood/src/triggers.cpp
	blood/src/view.cpp
	blood/src/warp.cpp
	blood/src/weapon.cpp

	common/utility/m_argv.cpp
	common/utility/files.cpp
	common/utility/files_decompress.cpp
	common/utility/zstring.cpp
	common/utility/zstrformat.cpp
	common/utility/utf8.cpp
	common/utility/superfasthash.cpp
	common/utility/configfile.cpp
	common/utility/file_zip.cpp
	common/utility/resourcefile.cpp
	common/utility/matrix.cpp
	common/utility/m_png.cpp
	common/utility/memarena.cpp

	common/textures/bitmap.cpp
	common/textures/texture.cpp
	common/textures/image.cpp
	common/textures/imagetexture.cpp
	common/textures/formats/buildtexture.cpp
	common/textures/formats/ddstexture.cpp
	common/textures/formats/jpegtexture.cpp
	common/textures/formats/pcxtexture.cpp
	common/textures/formats/pngtexture.cpp
	common/textures/formats/tgatexture.cpp
	
)

if( MSVC )
	enable_precompiled_headers( g_pch.h PCH_SOURCES )
	# The original Build code was written with unsigned chars and unfortunately they still haven't been eliminated entirely.
	# All other code should stay with signed chars. What a mess... :(
	set_source_files_properties( ${PCH_SOURCES} PROPERTIES COMPILE_FLAGS "/J" )
else()
	# Temporary solution for compilers other than MSVC
	set_source_files_properties( ${PCH_SOURCES} PROPERTIES COMPILE_FLAGS "-include g_pch.h" )
endif()

add_executable( demolition WIN32 MACOSX_BUNDLE
	${HEADER_FILES}
	${NOT_COMPILED_SOURCE_FILES}
	#__autostart.cpp
	${SYSTEM_SOURCES}
	${FASTMATH_SOURCES}
	${PCH_SOURCES}
	#utility/strnatcmp.c
	#utility/zstring.cpp
	#zzautozend.cpp
)

set_source_files_properties( ${FASTMATH_SOURCES} PROPERTIES COMPILE_FLAGS ${DEM_FASTMATH_FLAG} )
set_source_files_properties( xlat/parse_xlat.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xlat_parser.c" )
set_source_files_properties( utility/sc_man.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h" )
set_source_files_properties( ${NOT_COMPILED_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE )


if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
	# [BL] Solaris requires these to be explicitly linked.
	set( DEMOLITION_LIBS ${DEMOLITION_LIBS} nsl socket)
endif()

if( UNIX )
	find_package( Backtrace )
	if(Backtrace_FOUND)
		set( DEMOLITION_LIBS ${DEMOLITION_LIBS} ${Backtrace_LIBRARIES} )
	endif()
endif()

target_link_libraries( demolition ${DEMOLITION_LIBS} enet gdtoa dumb lzma )

include_directories(
    build/include
	mact/include
	audiolib/include
	glad/include
	libxmp-lite/include
	libxmp-lite/include/libxmp-lite
	libsmackerdec/include
	thirdparty/include
	common
	common/utility
	common/console
	common/textures
	platform

	${CMAKE_BINARY_DIR}/libraries/gdtoa
	${CMAKE_BINARY_DIR}/libraries/enet

	#${SYSTEM_SOURCES_DIR} 
)

add_dependencies( demolition revision_check )

# Due to some quirks, we need to do this in this order
if( NOT DEMOLITION_OUTPUT_OLDSTYLE )
	# RUNTIME_OUTPUT_DIRECTORY does not exist in CMake 2.4.
	# Linux distributions are slow to adopt 2.6. :(
	set_target_properties( demolition PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DEMOLITION_OUTPUT_DIR} )
	set_target_properties( demolition PROPERTIES OUTPUT_NAME ${DEMOLITION_EXE_NAME} )
else()
	set_target_properties( demolition PROPERTIES
		RUNTIME_OUTPUT_NAME ${DEMOLITION_EXE_NAME}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEMOLITION_OUTPUT_DIR}
		RUNTIME_OUTPUT_NAME_DEBUG ${DEMOLITION_EXE_NAME}d
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEMOLITION_OUTPUT_DIR}
		RUNTIME_OUTPUT_NAME_MINSIZEREL ${DEMOLITION_EXE_NAME}msr
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${DEMOLITION_OUTPUT_DIR}
		RUNTIME_OUTPUT_NAME_RELWITHDEBINFO ${DEMOLITION_EXE_NAME}rd
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DEMOLITION_OUTPUT_DIR}
	)
endif()

if( MSVC )
	option( DEMOLITION_GENERATE_MAPFILE "Generate .map file for debugging." OFF )
	set( LINKERSTUFF "/MANIFEST:NO" )

	if( DEMOLITION_GENERATE_MAPFILE )
		set( LINKERSTUFF "${LINKERSTUFF} /MAP" )
	endif()
	set_target_properties(demolition PROPERTIES LINK_FLAGS ${LINKERSTUFF})

	add_custom_command(TARGET demolition POST_BUILD
		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\platform\\win32\\manifest.game.xml\" -outputresource:\"$<TARGET_FILE:demolition>\"\;\#1
		COMMENT "Adding manifest..."
	)

endif()

if( NOT WIN32 AND NOT APPLE )
	FILE( WRITE ${CMAKE_CURRENT_BINARY_DIR}/link-make "if [ ! -e ${DEMOLITION_OUTPUT_DIR}/${DEMOLITION_EXE_NAME} ]; then ln -sf ${CMAKE_CURRENT_BINARY_DIR}/${DEMOLITION_EXE_NAME} ${DEMOLITION_OUTPUT_DIR}/${DEMOLITION_EXE_NAME}; fi" )
	add_custom_command( TARGET demolition POST_BUILD
		COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/link-make
		COMMAND /bin/sh -c ${CMAKE_CURRENT_BINARY_DIR}/link-make )
endif()

#add_custom_command(TARGET demolition POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#	${CMAKE_SOURCE_DIR}/soundfont/gdemolition.sf2 $<TARGET_FILE_DIR:demolition>/soundfonts/gdemolition.sf2
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#	${CMAKE_SOURCE_DIR}/fm_banks/GENMIDI.GS.wopl $<TARGET_FILE_DIR:demolition>/fm_banks/GENMIDI.GS.wopl
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#	${CMAKE_SOURCE_DIR}/fm_banks/gs-by-papiezak-and-sneakernets.wopn $<TARGET_FILE_DIR:demolition>/fm_banks/gs-by-papiezak-and-sneakernets.wopn
#)

if( CMAKE_COMPILER_IS_GNUCXX )
	# GCC misoptimizes this file
	#set_source_files_properties( oplsynth/fmopl.cpp PROPERTIES COMPILE_FLAGS "-fno-tree-dominator-opts -fno-tree-fre" )
endif()
#if( DEM_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
	# Need to enable intrinsics for these files.
#	if( SSE_MATTERS )
#		set_source_files_properties(
#			utility/x86.cpp
#			PROPERTIES COMPILE_FLAGS "-msse2 -mmmx" )
#	endif()
#endif()

if( APPLE )
	set( LINK_FRAMEWORKS "-framework Cocoa -framework IOKit -framework OpenGL")

	if( HAVE_VULKAN )
		set( LINK_FRAMEWORKS "${LINK_FRAMEWORKS} -framework QuartzCore" )
	endif()

	set_target_properties(demolition PROPERTIES
		LINK_FLAGS "${LINK_FRAMEWORKS}"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/posix/osx/demolition-info.plist" )

	# Dymanic libraries like libvulkan.dylib or libMoltenVK.dylib will be loaded by dlopen()
	# if placed in the directory with the main executable
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rpath @executable_path" )
endif()

if( WIN32 )
	set( INSTALL_PATH . CACHE STRING "Directory where the demolition executable will be placed during install." )
else()
	set( INSTALL_PATH bin CACHE STRING "Directory where the demolition executable will be placed during install." )
endif()
install(TARGETS demolition
		DESTINATION ${INSTALL_PATH}
		COMPONENT "Game executable")

source_group("Utility" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/utility/.+")
source_group("Code\\Textures" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/textures/.+")
source_group("Code\\Textures\\Formats" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/common/textures/formats/.+")
source_group("Utility\\Audiolib" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/audiolib/.+")
source_group("Utility\\Audiolib Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/audiolib/include/.+")
source_group("Utility\\Audiolib Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/audiolib/src/.+")
source_group("Utility\\Glad" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glad/.+")
source_group("Utility\\Glad Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glad/include/glad/.+")
source_group("Utility\\Glad Khr Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glad/include/Khr/.+")
source_group("Utility\\Glad Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glad/src/.+")
source_group("Utility\\Smackerdec" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/smackerdec/.+")
source_group("Utility\\Smackerdec\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libsmackerdec/include/.+")
source_group("Utility\\Smackerdec\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libsmackerdec/src/.+")
source_group("Utility\\XMP-Lite" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/.+")
source_group("Utility\\XMP-Lite Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/include/libxmp-lite/.+")
source_group("Utility\\XMP-Lite Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/src/.+")
source_group("Utility\\Mact" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mact/.+")
source_group("Utility\\Mact Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mact/include/.+")
source_group("Utility\\Mact Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mact/src/.+")
source_group("Utility\\Third Party" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/.+")
source_group("Utility\\Third Party Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include/.+")
source_group("Utility\\Third Party Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/src/.+")
source_group("OpenGL Backend" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/glbackend/.+")
source_group("Build Engine" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/.+")
source_group("Build Engine\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/include/.+")
source_group("Build Engine\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/build/src/.+")
#source_group("Game Frontends\\EDuke32\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/duke3d/.h")
source_group("Game Frontends\\EDuke32\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/duke3d/.+")
#source_group("Game Frontends\\RedNukem\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/rr/.h")
source_group("Game Frontends\\RedNukem\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/rr/.+")
#source_group("Game Frontends\\NBlood\\Headers" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/blood/.h")
source_group("Game Frontends\\NBlood\\Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/blood/.+")
 