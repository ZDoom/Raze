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
#define MAXCLIPNUM 2048
#define CLIPCURBHEIGHT (1<<8)
typedef struct
{
    int32_t x1, y1, x2, y2;
} linetype;

extern int16_t clipsectorlist[MAXCLIPSECTORS];

int clipinsidebox(vec2_t *vect, int wallnum, int walldist);
inline int clipinsidebox(int x, int y, int wall, int dist)
{
    vec2_t v = { x, y };
    return clipinsidebox(&v, wall, dist);
}
int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist);


int32_t clipmove(vec3_t *const pos, int *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                 int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum = 3) ATTRIBUTE((nonnull(1, 2)));

int pushmove(vec3_t *const vect, int *const sectnum, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
                 uint32_t const cliptype, bool clear = true) ATTRIBUTE((nonnull(1, 2)));

#endif
