// Windows DIB/DirectDraw interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef __build_interface_layer__
#define __build_interface_layer__ WIN

extern int32_t backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive
extern uint32_t maxrefreshfreq;

extern int32_t glusecds;

extern char di_disabled;

int32_t win_gethwnd(void);
int32_t win_gethinstance(void);

void win_allowtaskswitching(int32_t onf);
int32_t win_checkinstance(void);

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

