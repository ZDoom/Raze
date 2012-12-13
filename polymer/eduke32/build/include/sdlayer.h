// SDL interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef __build_interface_layer__
#define __build_interface_layer__ SDL

#include "sdl_inc.h"
#include "compat.h"
#include "baselayer.h"

#ifdef _WIN32
HINSTANCE win_gethinstance(void);
#endif

struct sdlappicon {
	int32_t width,height;
	uint8_t *pixels;
};

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
int32_t SDL_WaitEventTimeout(SDL_Event *event, int32_t timeout);
#endif

static inline void idle_waitevent_timeout(uint32_t timeout)
{
    SDL_WaitEventTimeout(NULL, timeout);
}

static inline void idle_waitevent(void)
{
    SDL_WaitEvent(NULL);
}

static inline void idle(void)
{
#ifndef _WIN32
    usleep(1000);
#else
    Sleep(1);
#endif
}

#else
#if (__build_interface_layer__ != SDL)
#error "Already using the " __build_interface_layer__ ". Can't now use SDL."
#endif
#endif // __build_interface_layer__

