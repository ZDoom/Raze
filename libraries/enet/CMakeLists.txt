cmake_minimum_required( VERSION 2.8.7 )

make_release_only()

if( DEM_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -fomit-frame-pointer" )
endif()

if (MSVC)
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4244 /wd4018 /wd4267" ) # this code throws a lot of warnings.
endif()

add_library( enet STATIC
	callbacks.c
	compress.c
	host.c
	list.c
	packet.c
	peer.c
	protocol.c
	unix.c		# This and the next one are platform safe!
	win32.c
     )
target_link_libraries( enet )
