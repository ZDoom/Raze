#pragma once

#ifndef clip_h_
#define clip_h_

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCLIPNUM 1024

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

extern clipinfo_t clipinfo[CM_MAX];

typedef struct
{
    int16_t numsectors, numwalls;
    usectortype *sector;
    uwalltype *wall;
} mapinfo_t;

extern mapinfo_t origmapinfo, clipmapinfo;

extern void clipmapinfo_init();
extern int32_t quickloadboard;
extern int16_t *sectq;
extern int16_t pictoidx[MAXTILES];  // maps tile num to clipinfo[] index
extern int16_t clipspritelist[MAXCLIPNUM];
extern void mapinfo_set(mapinfo_t *bak, mapinfo_t *newmap);
extern int32_t clipsprite_try(uspritetype const * const spr, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax);
extern int32_t clipsprite_initindex(int32_t curidx, uspritetype const * const curspr, int32_t *clipsectcnt, const vec3_t *vect);

#endif // HAVE_CLIPSHAPE_FEATURE
typedef struct
{
    int32_t x1, y1, x2, y2;
} linetype;

extern linetype clipit[MAXCLIPNUM];

extern int16_t clipnum;
extern int32_t clipsectnum, origclipsectnum, clipspritenum;
extern int16_t clipsectorlist[MAXCLIPNUM], origclipsectorlist[MAXCLIPNUM];

int clipinsidebox(vec2_t *vect, int wallnum, int walldist);
int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist);

extern int32_t clipmoveboxtracenum;

int32_t clipmove(vec3_t *vect, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist, int32_t ceildist,
    int32_t flordist, uint32_t cliptype) ATTRIBUTE((nonnull(1, 2)));
int32_t clipmovex(vec3_t *pos, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist, int32_t ceildist,
    int32_t flordist, uint32_t cliptype, uint8_t noslidep) ATTRIBUTE((nonnull(1, 2)));
int32_t pushmove(vec3_t *vect, int16_t *sectnum, int32_t walldist, int32_t ceildist, int32_t flordist,
    uint32_t cliptype) ATTRIBUTE((nonnull(1, 2)));

#ifdef __cplusplus
}
#endif

#endif
