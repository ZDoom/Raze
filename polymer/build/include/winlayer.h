// Windows DIB/DirectDraw interface layer
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __build_interface_layer__
#define __build_interface_layer__ WIN

extern int backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive
extern unsigned maxrefreshfreq;

extern int glusecds;

long win_gethwnd(void);
long win_gethinstance(void);

void win_allowtaskswitching(int onf);
int win_checkinstance(void);

#include "baselayer.h"

#else
#if (__build_interface_layer__ != WIN)
#error "Already using the " __build_interface_layer__ ". Can't now use Windows."
#endif
#endif // __build_interface_layer__

