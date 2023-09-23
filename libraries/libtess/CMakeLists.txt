make_release_only()


set( TESS_FILES
	Source/bucketalloc.c
	Source/dict.c
	Source/geom.c
	Source/mesh.c
	Source/priorityq.c
	Source/sweep.c
	Source/tess.c
 )

include_directories( Include )

add_library( tess2 STATIC ${TESS_FILES} )
target_link_libraries( tess2 )
