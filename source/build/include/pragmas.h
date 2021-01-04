// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "templates.h"
#ifndef pragmas_h_
#define pragmas_h_

static inline constexpr int ksgn(int32_t a) { return (a > 0) - (a < 0); }

inline int sgn(int32_t a) { return (a > 0) - (a < 0); }

#endif  // pragmas_h_
