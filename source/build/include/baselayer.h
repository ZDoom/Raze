// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#pragma once

#ifndef baselayer_h_
#define baselayer_h_

#include "compat.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "inputstate.h"
#include "printf.h"
#include "zstring.h"
#include "vectors.h"

EXTERN_CVAR(Bool, r_usenewaspect)

// video
extern int32_t newaspect_enable;

void    videoShowFrame(int32_t);

#include "gamestruct.h"
#endif // baselayer_h_

