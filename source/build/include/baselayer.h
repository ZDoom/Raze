// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#pragma once

#ifndef baselayer_h_
#define baselayer_h_

#include "compat.h"
#include "osd.h"
#include "timer.h"
#include "c_cvars.h"
#include "inputstate.h"
#include "printf.h"
#include "zstring.h"
#include "vectors.h"

extern int32_t swapcomplete;

EXTERN_CVAR(Bool, r_usenewaspect)

// video
extern int32_t newaspect_enable;
extern int32_t r_fpgrouscan;
extern int32_t setaspect_new_use_dimen;
extern int32_t xres, yres, bpp;
extern double refreshfreq;

int32_t videoCheckMode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced);
int32_t videoSetMode(int32_t x, int32_t y, int32_t c, int32_t fs);
void    videoGetModes(void);
void    videoShowFrame(int32_t);
int32_t videoUpdatePalette(int32_t start, int32_t num);

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

extern int32_t g_logFlushWindow;

#include "gamestruct.h"
#endif // baselayer_h_

