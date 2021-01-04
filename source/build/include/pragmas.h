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

extern int32_t reciptable[2048];

static inline int32_t scale(int32_t eax, int32_t edx, int32_t ecx)
{
    return int64_t(eax) * edx / ecx;
}

static inline constexpr int ksgn(int32_t a) { return (a > 0) - (a < 0); }

inline int sgn(int32_t a) { return (a > 0) - (a < 0); }

static inline int32_t krecipasm(int32_t i)
{
    // Ken did this
    union { int32_t i; float f; } x;
    x.f = (float)i;
    i = x.i;
    return ((reciptable[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^ (i >> 31));
}

#endif  // pragmas_h_
