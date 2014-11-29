
#include "compat.h"
#include "sdlappicon.h"
#include "sdl_inc.h"

static uint8_t sdlappicon_pixels[] = {
#if defined _WIN32 && SDL_MAJOR_VERSION==1
# include "eduke32_icon_32px.c"
#else
# include "eduke32_icon_48px.c"
#endif
};

struct sdlappicon sdlappicon = {
#if defined _WIN32 && SDL_MAJOR_VERSION==1
	32,32,
#else
	48,48,
#endif
	sdlappicon_pixels
};
