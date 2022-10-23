// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#ifndef clip_h_
#define clip_h_

#define MAXCLIPSECTORS 512
#define CLIPCURBHEIGHT (1)
struct CollisionBase;

CollisionBase clipmove_(vec3_t *const pos, int *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                 int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum = 3) ATTRIBUTE((nonnull(1, 2)));


#endif
