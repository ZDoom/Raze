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

extern int32_t clipmoveboxtracenum;

int32_t clipmove(vec3_t *const pos, int16_t *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                 int32_t const flordist, uint32_t const cliptype) ATTRIBUTE((nonnull(1, 2)));

inline int clipmove(int* x, int* y, int* z, short* sect, int xv, int yv, int wal, int ceil, int flor, int ct)
{
    vec3_t xyz = { *x,*y,*z };
    int retval = clipmove(&xyz, sect, xv, yv, wal, ceil, flor, ct);
    *x = xyz.x;
    *y = xyz.y;
    *z = xyz.z;
    return retval;
}

int32_t clipmovex(vec3_t *const pos, int16_t *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                  int32_t const flordist, uint32_t const cliptype, uint8_t const noslidep) ATTRIBUTE((nonnull(1, 2)));
int pushmove(vec3_t *const vect, int16_t *const sectnum, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
                 uint32_t const cliptype, bool clear = true) ATTRIBUTE((nonnull(1, 2)));

inline int pushmove(int* x, int* y, int* z, int16_t* const sectnum, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
    uint32_t const cliptype, bool clear = true)
{
    vec3_t v = { *x,*y,*z };
    auto r = pushmove(&v, sectnum, walldist, ceildist, flordist, cliptype, clear);
    *x = v.x;
    *y = v.y;
    *z = v.z;
    return r;
}

#endif
