// SDL interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef __build_interface_layer__
#define __build_interface_layer__ SDL

#include "baselayer.h"

struct sdlappicon {
	int32_t width,height;
	uint32_t *pixels;
	uint8_t *mask;
};

#else
#if (__build_interface_layer__ != SDL)
#error "Already using the " __build_interface_layer__ ". Can't now use SDL."
#endif
#endif // __build_interface_layer__

