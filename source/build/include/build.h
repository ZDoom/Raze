// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#ifndef build_h_
#define build_h_

#define TRANSPARENT_INDEX 0

static_assert('\xff' == 255, "Char must be unsigned!");

#include "printf.h"
#include "palette.h"
#include "buildtiles.h"
#include "c_cvars.h"
#include "cmdlib.h"

typedef int64_t coord_t;

enum
{
    MAXWALLSB = 6144,

    // Maximum number of component tiles in a multi-psky:
    MAXUNIQHUDID = 256, //Extra slots so HUD models can store animation state without messing game sprites

    TSPR_TEMP = 99,

    CLIPMASK0 = (1 << 16) + 1,
    CLIPMASK1 = (256 << 16) + 64
};



#define POINT2(i) (wall[wall[i].point2])


#include "maptypes.h"
#include "clip.h"

int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out);

struct usermaphack_t 
{
    FString mhkfile;
    FString title;
    uint8_t md4[16]{};
};

enum {
    PALETTE_MAIN = 1<<0,
    PALETTE_SHADE = 1<<1,
    PALETTE_TRANSLUC = 1<<2,
};

enum {
    ENGINECOMPATIBILITY_NONE = 0,
    ENGINECOMPATIBILITY_19950829, // Powerslave/Exhumed
    ENGINECOMPATIBILITY_19960925, // Blood v1.21
    ENGINECOMPATIBILITY_19961112, // Duke 3d v1.5, Redneck Rampage
};

inline int leveltimer;
inline int32_t Numsprites;
inline int32_t display_mirror;
inline int32_t randomseed;
inline uint8_t paletteloaded;
inline int32_t g_visibility = 512, g_relvisibility = 0;
inline int32_t enginecompatibility_mode;


void setVideoMode();

class F2DDrawer;


struct HitInfoBase;

inline int32_t ksqrt(uint64_t num)
{
    return int(sqrt(double(num)));
}

inline constexpr uint32_t uhypsq(int32_t const dx, int32_t const dy)
{
    return (uint32_t)dx*dx + (uint32_t)dy*dy;
}

static inline int64_t compat_maybe_truncate_to_int32(int64_t val)
{
    return enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? (int32_t)val : val;
}

#endif // build_h_
