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

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCLIPSECTORS 512
#define MAXCLIPNUM 2048

#ifdef HAVE_CLIPSHAPE_FEATURE

#define CM_MAX 256  // must be a power of 2

// sectoidx bits
#undef CM_NONE
#define CM_NONE (CM_MAX<<1)
#define CM_SOME (CM_NONE-1)
#define CM_OUTER (CM_MAX)   // sector surrounds clipping sector

// sprite -> sector tag mappings
#define CM_XREPEAT floorpal
#define CM_YREPEAT floorxpanning
#define CM_XOFFSET ceilingshade
#define CM_YOFFSET floorshade
#define CM_CSTAT hitag
#define CM_ANG extra
#define CM_FLOORZ(Sec) (*(int32_t *)&sector[Sec].ceilingxpanning)  // ceilingxpanning,ceilingypanning,floorpicnum
#define CM_CEILINGZ(Sec) (*(int32_t *)&sector[Sec].visibility)  // visibility,fogpal,lotag

// backup of original normalized coordinates
#define CM_WALL_X(Wal) (*(int32_t *)&wall[Wal].picnum)  // picnum, overpicnum
#define CM_WALL_Y(Wal) (*(int32_t *)&wall[Wal].lotag)  // lotag, hitag

// don't rotate when applying clipping, for models with rotational symmetry
#define CM_NOROT(Spri) (sprite[Spri].cstat&2)
#define CM_NOROTS(Sect) (sector[Sect].CM_CSTAT&2)

typedef struct
{
    int16_t qbeg, qend;  // indices into sectq
    int16_t picnum, next;
    int32_t maxdist;
} clipinfo_t;

typedef struct
{
    int16_t numsectors, numwalls;
    usectortype *sector;
    uwalltype *wall;
} mapinfo_t;

extern int32_t quickloadboard;
extern void engineInitClipMaps();
extern void engineSetClipMap(mapinfo_t *bak, mapinfo_t *newmap);

#endif // HAVE_CLIPSHAPE_FEATURE
typedef struct
{
    int32_t x1, y1, x2, y2;
} linetype;

extern int16_t clipsectorlist[MAXCLIPSECTORS];

int clipinsidebox(vec2_t *vect, int wallnum, int walldist);
int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist);

int sectoradjacent(int sect1, int sect2);

extern int32_t clipmoveboxtracenum;

int32_t clipmove(vec3_t *const pos, int16_t *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                 int32_t const flordist, uint32_t const cliptype) ATTRIBUTE((nonnull(1, 2)));
int32_t clipmovex(vec3_t *const pos, int16_t *const sectnum, int32_t xvect, int32_t yvect, int32_t const walldist, int32_t const ceildist,
                  int32_t const flordist, uint32_t const cliptype, uint8_t const noslidep) ATTRIBUTE((nonnull(1, 2)));
int32_t pushmove(vec3_t *const vect, int16_t *const sectnum, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
                 uint32_t const cliptype) ATTRIBUTE((nonnull(1, 2)));

#ifdef __cplusplus
}
#endif

#endif
