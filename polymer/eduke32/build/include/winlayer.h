// Windows DIB/DirectDraw interface layer
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __build_interface_layer__
#define __build_interface_layer__ WIN

extern int32_t backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive
extern unsigned maxrefreshfreq;

extern int32_t glusecds;

int32_t win_gethwnd(void);
int32_t win_gethinstance(void);

void win_allowtaskswitching(int32_t onf);
int32_t win_checkinstance(void);

#include "baselayer.h"

#else
#if (__build_interface_layer__ != WIN)
#error "Already using the " __build_interface_layer__ ". Can't now use Windows."
#endif
#endif // __build_interface_layer__

