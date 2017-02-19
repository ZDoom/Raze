// Windows DIB/DirectDraw interface layer for the Build Engine
// Originally by Jonathon Fowler (jf@jonof.id.au)

#ifndef build_interface_layer_
#define build_interface_layer_ WIN

#include "windows_inc.h"

extern uint32_t maxrefreshfreq;

extern int32_t glusecds;

extern char di_disabled;
extern char forcegl;

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
#if (build_interface_layer_ != WIN)
#error "Already using the " build_interface_layer_ ". Can't now use Windows."
#endif
#endif // build_interface_layer_

