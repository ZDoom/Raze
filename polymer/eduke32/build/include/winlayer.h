// Windows DIB/DirectDraw interface layer for the Build Engine
// Originally by Jonathon Fowler (jf@jonof.id.au)

#ifndef __build_interface_layer__
#define __build_interface_layer__ WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern uint32_t maxrefreshfreq;

extern int32_t glusecds;

extern char di_disabled;

HWND win_gethwnd(void);
HINSTANCE win_gethinstance(void);

extern void idle_waitevent_timeout(uint32_t timeout);

static inline void idle_waitevent(void)
{
    idle_waitevent_timeout(100);
}

static inline void idle(void)
{
    idle_waitevent();
}

#include "baselayer.h"

#else
#if (__build_interface_layer__ != WIN)
#error "Already using the " __build_interface_layer__ ". Can't now use Windows."
#endif
#endif // __build_interface_layer__

