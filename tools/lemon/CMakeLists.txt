cmake_minimum_required( VERSION 3.1.0 )

if( NOT CMAKE_CROSSCOMPILING )
	set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG" )

if (MSVC)
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244 /wd4267" )
endif()

	add_executable( lemon lemon.c )
	set( CROSS_EXPORTS ${CROSS_EXPORTS} lemon PARENT_SCOPE )

	# Lemon wants lempar.c in its directory
	add_custom_command( TARGET lemon
		POST_BUILD
		COMMAND echo $<TARGET_FILE_DIR:lemon>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/lempar.c $<TARGET_FILE_DIR:lemon> )	
endif()
