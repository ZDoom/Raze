// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "a.h"
#include "build.h"
#include "baselayer.h"
#include "engine_priv.h"

int16_t clipnum;
linetype clipit[MAXCLIPNUM];
int32_t clipsectnum, origclipsectnum, clipspritenum;
int16_t clipsectorlist[MAXCLIPSECTORS], origclipsectorlist[MAXCLIPSECTORS];
#ifdef HAVE_CLIPSHAPE_FEATURE
int16_t clipspritelist[MAXCLIPNUM];  // sector-like sprite clipping
#endif
static int16_t clipobjectval[MAXCLIPNUM];

////// sector-like clipping for sprites //////
#ifdef HAVE_CLIPSHAPE_FEATURE
void engineSetClipMap(mapinfo_t *bak, mapinfo_t *newmap)
{
    if (bak)
    {
        bak->numsectors = numsectors;
        bak->numwalls = numwalls;
        bak->sector = (usectortype *) sector;
        bak->wall = (uwalltype *) wall;
    }

    if (newmap)
    {
        numsectors = newmap->numsectors;
        numwalls = newmap->numwalls;
        sector = (sectortype *) newmap->sector;
        wall = (walltype *) newmap->wall;
    }
}

mapinfo_t origmapinfo, clipmapinfo;
int32_t quickloadboard=0;

clipinfo_t clipinfo[CM_MAX];
static int32_t numclipmaps;

static int32_t numclipsects;  // number in sectq[]
static int16_t *sectoidx;
int16_t *sectq;  // [numsectors]
int16_t pictoidx[MAXTILES];  // maps tile num to clipinfo[] index
static int16_t *tempictoidx;

static usectortype *loadsector;
static uwalltype *loadwall, *loadwallinv;
static uspritetype *loadsprite;

static vec2_t const hitscangoal = { (1<<29)-1, (1<<29)-1 };
#ifdef USE_OPENGL
int32_t hitallsprites = 0;
#endif

void engineInitClipMaps()
{
    numclipmaps = 0;
    numclipsects = 0;

    DO_FREE_AND_NULL(sectq);
    DO_FREE_AND_NULL(sectoidx);
    DO_FREE_AND_NULL(tempictoidx);
    DO_FREE_AND_NULL(loadsector);
    DO_FREE_AND_NULL(loadwall);
    DO_FREE_AND_NULL(loadwallinv);
    DO_FREE_AND_NULL(loadsprite);

    // two's complement trick, -1 = 0xff
    Bmemset(&pictoidx, -1, sizeof(pictoidx));
    Bmemset(&clipmapinfo, 0, sizeof(mapinfo_t));

    numsectors = 0;
    numwalls = 0;
}

// loads the clip maps.
// this should be called before any real map is loaded.
int32_t engineLoadClipMaps(void)
{
    int32_t i, k, w;

    int32_t lwcp = 0;
    size_t fi;
    size_t const g_clipMapFilesNum = g_clipMapFiles.size();

    int32_t *fisec = NULL;
    int32_t *fispr = NULL;

    int32_t ournumsectors=0, ournumwalls=0, ournumsprites=0;

    engineInitClipMaps();

    loadsector = (usectortype *) Xmalloc(MAXSECTORS * sizeof(sectortype));
    loadwall = (uwalltype *) Xmalloc(MAXWALLS * sizeof(walltype));
    loadsprite = (uspritetype *) Xmalloc(MAXSPRITES * sizeof(spritetype));

    if (g_clipMapFilesNum)
    {
        fisec = (int32_t *) Xcalloc(g_clipMapFilesNum, sizeof(int32_t));
        fispr = (int32_t *) Xcalloc(g_clipMapFilesNum, sizeof(int32_t));
    }

    quickloadboard = 1;
    for (fi = 0; fi < g_clipMapFilesNum; ++fi)
    {
        int16_t ang, cs;
        vec3_t tmppos;

        fisec[fi] = ournumsectors;
        fispr[fi] = ournumsprites;

        i = engineLoadBoard(g_clipMapFiles[fi], 8, &tmppos, &ang, &cs);
        if (i<0)
            continue;
        // Numsprites will now be set!

        initprintf("Loading clip map: %s\n", g_clipMapFiles[fi]);

        if (ournumsectors+numsectors>MAXSECTORS ||
            ournumwalls+numwalls>MAXWALLS ||
            ournumsprites+Numsprites>MAXSPRITES)
        {
            initprintf("clip map: warning: exceeded limits when loading %s, aborting.\n", g_clipMapFiles[fi]);
            break;
        }

        Bmemcpy(loadsector+ournumsectors, sector, numsectors*sizeof(sectortype));
        Bmemcpy(loadwall+ournumwalls, wall, numwalls*sizeof(walltype));
        Bmemcpy(loadsprite+ournumsprites, sprite, Numsprites*sizeof(spritetype));
        for (i=ournumsectors; i<ournumsectors+numsectors; i++)
            loadsector[i].wallptr += ournumwalls;
        for (i=ournumwalls; i<ournumwalls+numwalls; i++)
        {
            if (loadwall[i].point2>=0)
                loadwall[i].point2 += ournumwalls;
            if (loadwall[i].nextwall>=0)
            {
                loadwall[i].nextwall += ournumwalls;
                loadwall[i].nextsector += ournumsectors;
            }
        }
        for (i=ournumsprites; i<ournumsprites+Numsprites; i++)
            if (loadsprite[i].sectnum>=0)
                loadsprite[i].sectnum += ournumsectors;
        ournumsectors += numsectors;
        ournumwalls += numwalls;
        ournumsprites += Numsprites;

        ++lwcp;
    }
    quickloadboard = 0;

    if (ournumsectors==0 || ournumwalls==0 || ournumsprites==0)  // nothing loaded
    {
        engineInitClipMaps();

        Bfree(fisec);
        Bfree(fispr);

        return -1;
    }

    // shrink
    loadsector = (usectortype *) Xrealloc(loadsector, ournumsectors*sizeof(sectortype));
    loadwall = (uwalltype *) Xrealloc(loadwall, ournumwalls*sizeof(walltype));

    Bmemcpy(sector, loadsector, ournumsectors*sizeof(sectortype));
    Bmemcpy(wall, loadwall, ournumwalls*sizeof(walltype));
    Bmemcpy(sprite, loadsprite, ournumsprites*sizeof(spritetype));
    numsectors = ournumsectors;
    numwalls = ournumwalls;

    //  vvvv    don't use headsprite[sect,stat]!   vvvv

    sectoidx = (int16_t *) Xmalloc(numsectors*sizeof(sectoidx[0]));

    for (i=0; i<numsectors; i++)
        sectoidx[i] = CM_NONE;

    // determine outer sectors
    for (i=0; i<numsectors; i++)
    {
        for (w=sector[i].wallptr; w<sector[i].wallptr+sector[i].wallnum; w++)
            if (wall[w].nextsector<0)
            {
                sectoidx[i] = CM_OUTER;
                break;
            }
    }
    // break connections between outer sectors
    for (i=0; i<numsectors; i++)
    {
        if (sectoidx[i] == CM_OUTER)
            for (w=sector[i].wallptr; w<sector[i].wallptr+sector[i].wallnum; w++)
            {
                k = wall[w].nextwall;
                if (k>=0 && sectoidx[wall[w].nextsector]==CM_OUTER)
                {
                    wall[k].nextwall = wall[k].nextsector = -1;
                    wall[w].nextwall = wall[w].nextsector = -1;
                }
            }
    }

    {
        int16_t ns, outersect;
        int32_t pn, scnt, x, y, z, maxdist;

        sectq = (int16_t *) Xmalloc(numsectors*sizeof(sectq[0]));
        tempictoidx = (int16_t *) Xmalloc(MAXTILES*sizeof(tempictoidx[0]));

        for (i=0; i<MAXTILES; i++)
            tempictoidx[i]=-1;

        // collect sprite picnums
        for (i=0; i<MAXSPRITES && sprite[i].statnum<MAXSTATUS; i++)
        {
            pn = sprite[i].picnum;
            k = sprite[i].sectnum;
            //    -v-  note the <=                         ignore sprites in outer sectors
            if (pn<=0 || pn>=MAXTILES || k<0 || k>=numsectors || (sectoidx[k]&CM_OUTER))
                continue;

            if (numclipmaps >= CM_MAX)
            {
                initprintf("warning: reached max clip map number %d, not processing any more\n", CM_MAX);
                break;
            }

            // chain
            if (pictoidx[pn]>=0)
            {
                if (sectoidx[k]&CM_SOME)
                {
                    for (fi = 0; fi < g_clipMapFilesNum; ++fi)
                        if (k>=fisec[fi])
                            break;
                    initprintf("clip map \"%s\": error: tried to chain picnum %d (sprite %d) in sector %d which"
                        " already belongs to picnum %d.\n", g_clipMapFiles[fi], pn, i-fispr[fi], k-fisec[fi],
                        clipinfo[sectoidx[k]].picnum);
                    engineInitClipMaps();

                    Bfree(fisec);
                    Bfree(fispr);

                    return 2;
                }

                // new one is front
                clipinfo[numclipmaps].next = pictoidx[pn];
                pictoidx[pn] = numclipmaps;
            }
            else
            {
                clipinfo[numclipmaps].next = -1;
                pictoidx[pn] = numclipmaps;
            }

            if (!CM_NOROT(i))
            {
                if (sprite[i].ang!=1536 && sprite[i].ang!=512)
                {
                    for (fi = 0; fi < g_clipMapFilesNum; ++fi)
                        if (i>=fispr[fi])
                            break;
                    initprintf("clip map \"%s\": warning: sprite %d pointing neither northward nor southward. %s will be wrong.\n",
                        g_clipMapFiles[fi], i-fispr[fi], (sprite[i].cstat&48)==32 ? "Scaling and flipping" : "X-flipping");
                }
            }

            clipinfo[numclipmaps].picnum = pn;

            // collect sectors
            scnt = numclipsects;
            sectq[numclipsects++] = k;
            sectoidx[k] = numclipmaps;

            clipinfo[numclipmaps].qbeg = scnt;

            outersect = -1;

            do
            {
                k = sectq[scnt];

                for (w=sector[k].wallptr; w<sector[k].wallptr+sector[k].wallnum; w++)
                {
                    ns = wall[w].nextsector;
                    if (ns>=0)
                    {
                        if (sectoidx[ns]==CM_NONE)
                        {
                            sectoidx[ns] = numclipmaps;
                            sectq[numclipsects++] = ns;
                        }
                        else if (sectoidx[ns]&CM_OUTER)
                        {
                            if (outersect>=0 && ns!=outersect)
                            {
                                for (fi = 0; fi < g_clipMapFilesNum; ++fi)
                                    if (ns>=fisec[fi])
                                        break;
                                initprintf("clip map \"%s\": error: encountered more than one outer sector (%d and %d)"
                                    " for sprite %d.\n", g_clipMapFiles[fi], outersect-fisec[fi], ns-fisec[fi], i-fispr[fi]);
                                engineInitClipMaps();

                                Bfree(fisec);
                                Bfree(fispr);

                                return 3;
                            }

                            outersect = ns;
                            sectoidx[outersect] |= numclipmaps;
                        }
                        else if (sectoidx[ns]!=numclipmaps)
                        {
                            for (fi = 0; fi < g_clipMapFilesNum; ++fi)
                                if (ns>=fisec[fi])
                                    break;
                            initprintf("clip map \"%s\": error: encountered sector %d belonging to index %d"
                                " while collecting sectors for sprite %d (index %d).\n",
                                g_clipMapFiles[fi], ns-fisec[fi], sectoidx[ns], i-fispr[fi], numclipmaps);
                            engineInitClipMaps();

                            Bfree(fisec);
                            Bfree(fispr);

                            return 4;
                        }
                    }
                }
            } while (++scnt < numclipsects);

            if (outersect==-1)
            {
                initprintf("clip map: INTERNAL ERROR: outersect==-1!\n");
                engineInitClipMaps();

                Bfree(fisec);
                Bfree(fispr);

                return 5;
            }

            sectq[numclipsects++] = outersect;  // last is outer
            clipinfo[numclipmaps].qend = numclipsects-1;

            // normalize
            maxdist = 0;

            for (scnt=clipinfo[numclipmaps].qbeg; scnt<=clipinfo[numclipmaps].qend; scnt++)
            {
                k = sectq[scnt];

                x = sprite[i].x;
                y = sprite[i].y;
                z = sprite[i].z;

                sector[k].floorz -= z;
                sector[k].ceilingz -= z;

                if (scnt==clipinfo[numclipmaps].qbeg)
                {
                    // backup sprite tags since we'll discard sprites later
                    sector[k].CM_XREPEAT = sprite[i].xrepeat;
                    sector[k].CM_YREPEAT = sprite[i].yrepeat;
                    sector[k].CM_XOFFSET = sprite[i].xoffset;
                    sector[k].CM_YOFFSET = sprite[i].yoffset;
                    sector[k].CM_CSTAT   = sprite[i].cstat;
                    sector[k].CM_ANG     = sprite[i].ang;
                }

                // backup floor and ceiling z
                CM_FLOORZ(k) = sector[k].floorz;
                CM_CEILINGZ(k) = sector[k].ceilingz;

                for (w=sector[k].wallptr; w<sector[k].wallptr+sector[k].wallnum; w++)
                {
                    wall[w].x -= x;
                    wall[w].y -= y;

                    if (scnt!=clipinfo[numclipmaps].qend)
                    {
                        if (CM_NOROT(i))
                        {
                            if (klabs(wall[w].x) > maxdist)
                                maxdist = klabs(wall[w].x);
                            if (klabs(wall[w].y) > maxdist)
                                maxdist = klabs(wall[w].y);
                        }
                        else
                        {
                            int32_t tmp = ksqrt(uhypsq(wall[w].x, wall[w].y));
                            if (tmp > maxdist)
                                maxdist = tmp;
                        }
                    }

                    // aliasing
                    if (wall[w].lotag>0 || wall[w].hitag>0)
                    {
                        int32_t ii;

                        if (wall[w].lotag>0 && wall[w].hitag>0)
                        {
                            if (wall[w].lotag > wall[w].hitag)
                                swapshort(&wall[w].lotag, &wall[w].hitag);

                            for (ii=wall[w].lotag; ii<wall[w].hitag; ii++)
                                tempictoidx[ii] = numclipmaps;
                        }
                        else if (wall[w].lotag>0)
                        {
                            if (wall[w].lotag<MAXTILES)
                                tempictoidx[wall[w].lotag] = numclipmaps;
                        }
                        else
                        {
                            if (wall[w].hitag<MAXTILES)
                                tempictoidx[wall[w].hitag] = numclipmaps;
                        }
                    }

                    CM_WALL_X(w) = wall[w].x;
                    CM_WALL_Y(w) = wall[w].y;
                }
            }

            clipinfo[numclipmaps].maxdist = maxdist;
            numclipmaps++;
        }
    }

    // yes, too much copying, but better than ugly code
    Bmemcpy(loadsector, sector, ournumsectors*sizeof(sectortype));
    Bmemcpy(loadwall, wall, ournumwalls*sizeof(walltype));

    // loadwallinv will contain all walls with inverted orientation for x/y-flip handling
    loadwallinv = (uwalltype *) Xmalloc(ournumwalls*sizeof(walltype));

    {
        int32_t j, loopstart, loopend, numloopwalls;

        // invert walls!
        loopstart = 0;
        for (j=0; j<ournumwalls; j++)
        {
            wall[j].nextsector = wall[j].nextwall = -1;

            if (wall[j].point2 < j)
            {
                loopend = j+1;
                numloopwalls = loopend-loopstart;

                if (numloopwalls<3)
                {
                    loopstart = loopend;
                    continue;
                }

                for (k=0; k<numloopwalls; k++)
                {
                    wall[loopstart+k].x = loadwall[loopstart + (numloopwalls+1-k)%numloopwalls].x;
                    wall[loopstart+k].y = loadwall[loopstart + (numloopwalls+1-k)%numloopwalls].y;

                    CM_WALL_X(loopstart+k) = wall[loopstart+k].x;
                    CM_WALL_Y(loopstart+k) = wall[loopstart+k].y;
                }

                loopstart = loopend;
            }
        }

        // reconstruct wall connections
        for (i=0; i<ournumsectors; i++)
        {
            for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
                checksectorpointer(j, i);
        }
    }
    Bmemcpy(loadwallinv, wall, ournumwalls*sizeof(walltype));

    clipmapinfo.numsectors = numsectors;
    clipmapinfo.sector = loadsector;
    clipmapinfo.numwalls = numwalls;
    clipmapinfo.wall = loadwall;

    for (i=0; i<MAXTILES; i++)
    {
        if (pictoidx[i]==-1 && tempictoidx[i]>=0)
            pictoidx[i]=tempictoidx[i];
    }

    DO_FREE_AND_NULL(loadsprite);
    DO_FREE_AND_NULL(tempictoidx);

    // don't let other code be distracted by the temporary map we constructed
    numsectors = 0;
    numwalls = 0;
    initspritelists();

    if (lwcp > 0)
        initprintf("Loaded clip map%s.\n", lwcp==1 ? "" : "s");

    Bfree(fisec);
    Bfree(fispr);

    return 0;
}


int clipshape_idx_for_sprite(uspritetype const * const curspr, int curidx)
{
     // per-sprite init
     curidx = (curidx < 0) ? pictoidx[curspr->picnum] : clipinfo[curidx].next;

     while (curidx >= 0 && (curspr->cstat & 32) != (sector[sectq[clipinfo[curidx].qbeg]].CM_CSTAT & 32))
         curidx = clipinfo[curidx].next;

     return curidx;
}
#else
int32_t clipshape_idx_for_sprite(uspritetype const * const curspr, int32_t curidx)
{
    (void)curspr;
    UNREFERENCED_PARAMETER(curidx);
    return -1;
}
#endif  // HAVE_CLIPSHAPE_FEATURE
////// //////

////////// CLIPMOVE //////////

int32_t clipmoveboxtracenum = 3;

//
// clipinsidebox
//
int clipinsidebox(vec2_t *vect, int wallnum, int walldist)
{
    int const    r   = walldist << 1;
    auto const * wal = (uwalltype *)&wall[wallnum];
    vec2_t const v1  = { wal->x + walldist - vect->x, wal->y + walldist - vect->y };
    wal              = (uwalltype *)&wall[wal->point2];
    vec2_t v2        = { wal->x + walldist - vect->x, wal->y + walldist - vect->y };

    if (((v1.x < 0) && (v2.x < 0)) || ((v1.y < 0) && (v2.y < 0)) || ((v1.x >= r) && (v2.x >= r)) || ((v1.y >= r) && (v2.y >= r)))
        return 0;

    v2.x -= v1.x; v2.y -= v1.y;

    if (v2.x * (walldist - v1.y) >= v2.y * (walldist - v1.x))  // Front
    {
        v2.x *= ((v2.x > 0) ? (0 - v1.y) : (r - v1.y));
        v2.y *= ((v2.y > 0) ? (r - v1.x) : (0 - v1.x));
        return v2.x < v2.y;
    }

    v2.x *= ((v2.x > 0) ? (r - v1.y) : (0 - v1.y));
    v2.y *= ((v2.y > 0) ? (0 - v1.x) : (r - v1.x));
    return (v2.x >= v2.y) << 1;
}


//
// clipinsideboxline
//
int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist)
{
    int const r = walldist << 1;

    x1 += walldist - x;
    x2 += walldist - x;

    if (((x1 < 0) && (x2 < 0)) || ((x1 >= r) && (x2 >= r)))
        return 0;

    y1 += walldist - y;
    y2 += walldist - y;

    if (((y1 < 0) && (y2 < 0)) || ((y1 >= r) && (y2 >= r)))
        return 0;

    x2 -= x1;
    y2 -= y1;

    if (x2 * (walldist - y1) >= y2 * (walldist - x1))  // Front
    {
        x2 *= ((x2 > 0) ? (0 - y1) : (r - y1));
        y2 *= ((y2 > 0) ? (r - x1) : (0 - x1));
        return x2 < y2;
    }

    x2 *= ((x2 > 0) ? (r - y1) : (0 - y1));
    y2 *= ((y2 > 0) ? (0 - x1) : (r - x1));
    return (x2 >= y2) << 1;
}


#ifdef HAVE_CLIPSHAPE_FEATURE
int32_t clipsprite_try(uspritetype const * const spr, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)
{
    // try and see whether this sprite's picnum has sector-like clipping data
    int32_t i = pictoidx[spr->picnum];
    // handle sector-like floor sprites separately
    while (i>=0 && (spr->cstat&32) != (clipmapinfo.sector[sectq[clipinfo[i].qbeg]].CM_CSTAT&32))
        i = clipinfo[i].next;

    if (i>=0)
    {
        int32_t maxcorrection = clipinfo[i].maxdist;
        const int32_t k = sectq[clipinfo[i].qbeg];

        if ((spr->cstat&CSTAT_SPRITE_ALIGNMENT)!=CSTAT_SPRITE_ALIGNMENT_FLOOR)
        {
            int32_t const tempint1 = clipmapinfo.sector[k].CM_XREPEAT;
            maxcorrection = divideu32_noinline(maxcorrection * (int32_t) spr->xrepeat, tempint1);
        }
        else
        {
            int32_t const tempint1 = clipmapinfo.sector[k].CM_XREPEAT;
            int32_t const tempint2 = clipmapinfo.sector[k].CM_YREPEAT;
            maxcorrection = max(divideu32_noinline(maxcorrection * (int32_t) spr->xrepeat, tempint1),
                divideu32_noinline(maxcorrection * (int32_t) spr->yrepeat, tempint2));
        }

        maxcorrection -= MAXCLIPDIST;

        if ((spr->x < xmin - maxcorrection) || (spr->y < ymin - maxcorrection) ||
            (spr->x > xmax + maxcorrection) || (spr->y > ymax + maxcorrection))
            return 1;

        if (clipspritenum < MAXCLIPNUM)
            clipspritelist[clipspritenum++] = spr-(uspritetype *)sprite;
        //initprintf("%d: clip sprite[%d]\n",clipspritenum,j);
        return 1;
    }

    return 0;
}

static int32_t clipmove_warned;

static void addclipsect(int const sectnum)
{
    if (EDUKE32_PREDICT_TRUE(clipsectnum < MAXCLIPSECTORS))
        clipsectorlist[clipsectnum++] = sectnum;
    else if (!clipmove_warned)
    {
        OSD_Printf("!!clipsectnum\n");
        clipmove_warned = 1;
    }
}

// return: -1 if curspr has x-flip xor y-flip (in the horizontal map plane!), 1 else
int32_t clipsprite_initindex(int32_t curidx, uspritetype const * const curspr, int32_t *clipsectcnt, const vec3_t *vect)
{
    int32_t k, daz = curspr->z;
    int32_t scalex, scaley, scalez, flipx, flipy;
    int32_t flipmul=1;

    const int32_t j = sectq[clipinfo[curidx].qbeg];
    const int32_t tempint1 = sector[j].CM_XREPEAT;
    const int32_t tempint2 = sector[j].CM_YREPEAT;

    const int32_t rotang = (curspr->ang - sector[j].CM_ANG)&2047;
    const int32_t dorot = !CM_NOROTS(j);

    if ((curspr->cstat&CSTAT_SPRITE_ALIGNMENT)!=CSTAT_SPRITE_ALIGNMENT_FLOOR)  // face/wall sprite
    {
        scalex = scaley = divscale22(curspr->xrepeat, tempint1);
        scalez = divscale22(curspr->yrepeat, tempint2);

        flipx = 1-((curspr->cstat&4)>>1);
        flipy = 1;
    }
    else
    {
        scalex = divscale22(curspr->xrepeat, tempint1);
        scaley = divscale22(curspr->yrepeat, tempint2);
        scalez = scalex;

        flipx = 1-((curspr->cstat&4)>>1);
        flipy = 1-((curspr->cstat&8)>>2);
    }

    if (dorot)
    {
        flipmul = flipx*flipy;
        if (flipmul==-1)
            wall = (walltype *) loadwallinv;
    }

    if ((curspr->cstat&128) != (sector[j].CM_CSTAT&128))
        daz += (((curspr->cstat&128)>>6)-1)*((tilesiz[curspr->picnum].y*curspr->yrepeat)<<1);

    *clipsectcnt = clipsectnum = 0;
    // init sectors for this index
    for (k=clipinfo[curidx].qbeg; k<=clipinfo[curidx].qend; k++)
    {
        int32_t const j   = sectq[k];
        auto const    sec = (usectortype *)&sector[j];

        int32_t const startwall = sec->wallptr, endwall = startwall+sec->wallnum;

        sec->floorz = daz + mulscale22(scalez, CM_FLOORZ(j));
        sec->ceilingz = daz + mulscale22(scalez, CM_CEILINGZ(j));
        //initprintf("sec %d: f=%d, c=%d\n", j, sec->floorz, sec->ceilingz);

        for (int w=startwall; w<endwall; w++)
        {
            auto wal=(uwalltype *)&wall[startwall];
            wal->x = mulscale22(scalex, CM_WALL_X(w));
            wal->y = mulscale22(scaley, CM_WALL_Y(w));

            if (dorot)
            {
                wal->x *= flipx;
                wal->y *= flipy;
                rotatepoint(zerovec, *(vec2_t *) wal, rotang, (vec2_t *) wal);
            }

            wal->x += curspr->x;
            wal->y += curspr->y;
        }

        if (inside(vect->x, vect->y, j)==1)
            addclipsect(j);
    }

    // add outer sector if not inside inner ones
    if (clipsectnum==0)
        addclipsect(sectq[k-1]);

    return flipmul;
}

#endif

static void addclipline(int32_t dax1, int32_t day1, int32_t dax2, int32_t day2, int32_t daoval)
{
    if (clipnum < MAXCLIPNUM)
    {
        clipit[clipnum].x1 = dax1; clipit[clipnum].y1 = day1;
        clipit[clipnum].x2 = dax2; clipit[clipnum].y2 = day2;
        clipobjectval[clipnum] = daoval;
        clipnum++;
    }
    else if (!clipmove_warned)
    {
        initprintf("!!clipnum\n");
        clipmove_warned = 2;
    }
}

static FORCE_INLINE void clipmove_tweak_pos(const vec3_t *pos, int32_t gx, int32_t gy, int32_t x1, int32_t y1, int32_t x2,
                                      int32_t y2, int32_t *daxptr, int32_t *dayptr)
{
    int32_t daz;

    if (rintersect(pos->x, pos->y, 0, gx, gy, 0, x1, y1, x2, y2, daxptr, dayptr, &daz) == -1)
    {
        *daxptr = pos->x;
        *dayptr = pos->y;
    }
}

// Returns: should clip?
static int32_t check_floor_curb(int32_t dasect, int32_t nextsect, int32_t flordist, int32_t posz,
                                int32_t dax, int32_t day)
{
    auto const    sec2 = (usectortype *)&sector[nextsect];
    int32_t const daz2 = getflorzofslope(nextsect, dax, day);

    return ((sec2->floorstat&1) == 0 &&  // parallaxed floor curbs don't clip
        posz >= daz2-(flordist-1) &&  // also account for desired z distance tolerance
        daz2 < getflorzofslope(dasect, dax, day)-(1<<8));  // curbs less tall than 256 z units don't clip
}

int32_t clipmovex(vec3_t *pos, int16_t *sectnum,
                  int32_t xvect, int32_t yvect,
                  int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype,
                  uint8_t noslidep)
{
    const int32_t oboxtracenum = clipmoveboxtracenum;

    if (noslidep)
        clipmoveboxtracenum = 1;
    int32_t ret = clipmove(pos, sectnum, xvect, yvect,
        walldist, ceildist, flordist, cliptype);
    clipmoveboxtracenum = oboxtracenum;

    return ret;
}

//
// raytrace (internal)
//
static inline int32_t raytrace(int32_t x3, int32_t y3, int32_t *x4, int32_t *y4)
{
    int32_t hitwall = -1;

    for (bssize_t z=clipnum-1; z>=0; z--)
    {
        const int32_t x1 = clipit[z].x1, x2 = clipit[z].x2, x21 = x2-x1;
        const int32_t y1 = clipit[z].y1, y2 = clipit[z].y2, y21 = y2-y1;

        int32_t topu = x21*(y3-y1) - (x3-x1)*y21;
        if (topu <= 0)
            continue;

        if (x21*(*y4-y1) > (*x4-x1)*y21)
            continue;

        const int32_t x43 = *x4-x3;
        const int32_t y43 = *y4-y3;

        if (x43*(y1-y3) > (x1-x3)*y43)
            continue;

        if (x43*(y2-y3) <= (x2-x3)*y43)
            continue;

        const int32_t bot = x43*y21 - x21*y43;
        if (bot == 0)
            continue;

        bssize_t cnt = 256, nintx, ninty;

        do
        {
            cnt--; if (cnt < 0) { *x4 = x3; *y4 = y3; return z; }
            nintx = x3 + scale(x43, topu, bot);
            ninty = y3 + scale(y43, topu, bot);
            topu--;
        } while (x21*(ninty-y1) <= (nintx-x1)*y21);

        if (klabs(x3-nintx)+klabs(y3-ninty) < klabs(x3-*x4)+klabs(y3-*y4))
        {
            *x4 = nintx; *y4 = ninty; hitwall = z;
        }
    }

    return hitwall;
}

//
// keepaway (internal)
//
static inline void keepaway(int32_t *x, int32_t *y, int32_t w)
{
    const int32_t x1 = clipit[w].x1, dx = clipit[w].x2-x1;
    const int32_t y1 = clipit[w].y1, dy = clipit[w].y2-y1;
    const int32_t ox = ksgn(-dy), oy = ksgn(dx);
    char first = (klabs(dx) <= klabs(dy));

    do
    {
        if (dx*(*y-y1) > (*x-x1)*dy)
            return;

        if (first == 0)
            *x += ox;
        else
            *y += oy;

        first ^= 1;
    }
    while (1);
}

static int32_t get_floorspr_clipyou(int32_t x1, int32_t x2, int32_t x3, int32_t x4,
                                   int32_t y1, int32_t y2, int32_t y3, int32_t y4)
{
    int32_t clipyou = 0;

    if ((y1^y2) < 0)
    {
        if ((x1^x2) < 0) clipyou ^= (x1*y2 < x2*y1)^(y1<y2);
        else if (x1 >= 0) clipyou ^= 1;
    }
    if ((y2^y3) < 0)
    {
        if ((x2^x3) < 0) clipyou ^= (x2*y3 < x3*y2)^(y2<y3);
        else if (x2 >= 0) clipyou ^= 1;
    }
    if ((y3^y4) < 0)
    {
        if ((x3^x4) < 0) clipyou ^= (x3*y4 < x4*y3)^(y3<y4);
        else if (x3 >= 0) clipyou ^= 1;
    }
    if ((y4^y1) < 0)
    {
        if ((x4^x1) < 0) clipyou ^= (x4*y1 < x1*y4)^(y4<y1);
        else if (x4 >= 0) clipyou ^= 1;
    }

    return clipyou;
}

#if 0
static int sectoradjacent(int sect1, int sect2)
{
    if (sector[sect1].wallnum > sector[sect2].wallnum)
        swaplong(&sect1, &sect2);

    for (int i = 0; i < sector[sect1].wallnum; i++)
        if (wall[sector[sect1].wallptr+i].nextsector == sect2)
            return 1;

    return 0;
}
#endif

//
// clipmove
//
int32_t clipmove(vec3_t *pos, int16_t *sectnum, int32_t xvect, int32_t yvect,
                 int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return 0;

    uspritetype const * curspr=NULL;  // non-NULL when handling sprite with sector-like clipping

    int32_t const dawalclipmask = (cliptype & 65535);  // CLIPMASK0 = 0x00010001
    int32_t const dasprclipmask = (cliptype >> 16);    // CLIPMASK1 = 0x01000040

    vec2_t const move = { xvect, yvect };
    vec2_t       goal = { pos->x + (xvect >> 14), pos->y + (yvect >> 14) };
    vec2_t const cent = { (pos->x + goal.x) >> 1, (pos->y + goal.y) >> 1 };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff    = { goal.x - (pos->x), goal.y - (pos->y) };
    int32_t const rad     = nsqrtasm(uhypsq(diff.x, diff.y)) + MAXCLIPDIST + walldist + 8;
    vec2_t const  clipMin = { cent.x - rad, cent.y - rad };
    vec2_t const  clipMax = { cent.x + rad, cent.y + rad };

    int clipshapeidx  = -1;
    int clipsectcnt   = 0;
    int clipspritecnt = 0;

    clipsectorlist[0] = *sectnum;

    clipsectnum   = 1;
    clipnum       = 0;
    clipspritenum = 0;

    clipmove_warned = 0;

    do
    {
#ifdef HAVE_CLIPSHAPE_FEATURE
        if (clipsectcnt>=clipsectnum)
        {
            // one bunch of sectors completed (either the very first
            // one or a sector-like sprite one), prepare the next

            //initprintf("init sprite %d\n", clipspritecnt);
            if (!curspr)
            {
                // init sector-like sprites for clipping
                origclipsectnum = clipsectnum;
                Bmemcpy(origclipsectorlist, clipsectorlist, clipsectnum*sizeof(clipsectorlist[0]));

                // replace sector and wall with clip map
                engineSetClipMap(&origmapinfo, &clipmapinfo);
            }

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            clipshapeidx = clipshape_idx_for_sprite(curspr, clipshapeidx);

            if (clipshapeidx < 0)
            {
                clipspritecnt++;
                continue;
            }

            clipsprite_initindex(clipshapeidx, curspr, &clipsectcnt, pos);
        }
#endif

        int const dasect = clipsectorlist[clipsectcnt++];
        //if (curspr)
        //    initprintf("sprite %d/%d: sect %d/%d (%d)\n", clipspritecnt,clipspritenum, clipsectcnt,clipsectnum,dasect);

        ////////// Walls //////////

        auto const sec       = (usectortype *)&sector[dasect];
        int const  startwall = sec->wallptr;
        int const  endwall   = startwall + sec->wallnum;

        for (native_t j=startwall; j<endwall; j++)
        {
            auto const wal  = (uwalltype *)&wall[j];
            auto const wal2 = (uwalltype *)&wall[wal->point2];

            if ((wal->x < clipMin.x && wal2->x < clipMin.x) || (wal->x > clipMax.x && wal2->x > clipMax.x) ||
                (wal->y < clipMin.y && wal2->y < clipMin.y) || (wal->y > clipMax.y && wal2->y > clipMax.y))
                continue;

            vec2_t p1 = { wal->x, wal->y };
            vec2_t p2 = { wal2->x, wal2->y } ;
            vec2_t d  = { p2.x-p1.x, p2.y-p1.y };

            if (d.x * (pos->y-p1.y) < (pos->x-p1.x) * d.y)
                continue;  //If wall's not facing you

            vec2_t const r = { (d.y > 0) ? clipMax.x : clipMin.x, (d.x > 0) ? clipMin.y : clipMax.y };
            vec2_t       v = { d.x * (r.y - p1.y), d.y * (r.x - p1.x) };

            if (v.x >= v.y)
                continue;

            int clipyou = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
            if (curspr)
            {
                if (wal->nextsector>=0)
                {
                    const usectortype *sec2 = (usectortype *)&sector[wal->nextsector];

                    clipmove_tweak_pos(pos, diff.x, diff.y, p1.x, p1.y, p2.x, p2.y, &v.x, &v.y);

#define CLIPMV_SPR_F_DAZ2 getflorzofslope(wal->nextsector, v.x, v.y)
#define CLIPMV_SPR_F_BASEZ getflorzofslope(sectq[clipinfo[clipshapeidx].qend], v.x, v.y)

                    if ((sec2->floorstat&1) == 0)
                    {
                        if (CLIPMV_SPR_F_DAZ2-(flordist-1) <= pos->z && pos->z <= CLIPMV_SPR_F_BASEZ+(flordist-1))
                            clipyou = 1;
                    }

                    if (clipyou == 0)
                    {
#define CLIPMV_SPR_C_DAZ2 getceilzofslope(wal->nextsector, v.x, v.y)
#define CLIPMV_SPR_C_BASEZ getceilzofslope(sectq[clipinfo[clipshapeidx].qend], v.x, v.y)

                        if ((sec2->ceilingstat & 1) == 0)
                        {
                            if (CLIPMV_SPR_C_BASEZ-(ceildist-1) <= pos->z && pos->z <= CLIPMV_SPR_C_DAZ2+(ceildist-1))
                                clipyou = 1;
                        }
                    }
                }
            }
            else
#endif
                if (wal->nextsector < 0 || (wal->cstat&dawalclipmask))
                {
                    clipyou = 1;
#ifdef YAX_ENABLE
                    int const cb = yax_getbunch(dasect, YAX_CEILING);

                    if (cb >= 0 && (sec->ceilingstat & yax_waltosecmask(dawalclipmask)) == 0)
                    {
                        int const ynw = yax_getnextwall(j, YAX_CEILING);

                        if (ynw >= 0 && wall[ynw].nextsector >= 0 && (wall[ynw].cstat & dawalclipmask) == 0)
                        {
                            clipmove_tweak_pos(pos, diff.x, diff.y, p1.x, p1.y, p2.x, p2.y, &v.x, &v.y);
                            clipyou = check_floor_curb(dasect, wall[ynw].nextsector, flordist, pos->z, v.x, v.y);
                        }
                    }
#endif
                }
                else if (editstatus == 0)
                {
                    clipmove_tweak_pos(pos, diff.x, diff.y, p1.x, p1.y, p2.x, p2.y, &v.x, &v.y);
                    clipyou = check_floor_curb(dasect, wal->nextsector, flordist, pos->z, v.x, v.y);

                    if (clipyou == 0)
                    {
                        const usectortype *sec2 = (usectortype *)&sector[wal->nextsector];
                        int32_t daz2 = getceilzofslope(wal->nextsector, v.x, v.y);

                        clipyou = ((sec2->ceilingstat&1) == 0 &&
                                    pos->z <= daz2+(ceildist-1) &&
                                    daz2 > getceilzofslope(dasect, v.x, v.y)+(1<<8));
                    }
                }

            // We're not interested in any sector reached by portal traversal that we're "inside" of.
            if (clipsectcnt != 1 && inside(pos->x, pos->y, dasect) == 1) break;
            else if (clipyou)
            {
                int16_t objtype = int16_t(!curspr ?
                                          j + 32768 :
                                          curspr - (uspritetype *)sprite) + 49152;

                //Add 2 boxes at endpoints
                int32_t bsz = walldist; if (diff.x < 0) bsz = -bsz;
                addclipline(p1.x-bsz, p1.y-bsz, p1.x-bsz, p1.y+bsz, objtype);
                addclipline(p2.x-bsz, p2.y-bsz, p2.x-bsz, p2.y+bsz, objtype);
                bsz = walldist; if (diff.y < 0) bsz = -bsz;
                addclipline(p1.x+bsz, p1.y-bsz, p1.x-bsz, p1.y-bsz, objtype);
                addclipline(p2.x+bsz, p2.y-bsz, p2.x-bsz, p2.y-bsz, objtype);

                v.x = walldist; if (d.y > 0) v.x = -v.x;
                v.y = walldist; if (d.x < 0) v.y = -v.y;
                addclipline(p1.x+v.x, p1.y+v.y, p2.x+v.x, p2.y+v.y, objtype);
            }
            else if (wal->nextsector>=0)
            {
                if (inside(pos->x, pos->y, wal->nextsector) == 1) continue;

                int i;
                for (i=clipsectnum-1; i>=0; i--)
                    if (wal->nextsector == clipsectorlist[i]) break;
                if (i < 0) addclipsect(wal->nextsector);
            }
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
            continue;  // next sector of this index
#endif
        for (int j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[j];
            const int32_t cstat = spr->cstat;

            if ((cstat&dasprclipmask) == 0)
                continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
            if (clipsprite_try(spr, clipMin.x, clipMin.y, clipMax.x, clipMax.y))
                continue;
#endif
            vec2_t p1 = *(vec2_t const *)spr;

            switch (cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ALIGNMENT_FLOOR))
            {
            case CSTAT_SPRITE_ALIGNMENT_FACING:
                if (p1.x >= clipMin.x && p1.x <= clipMax.x && p1.y >= clipMin.y && p1.y <= clipMax.y)
                {
                    int32_t height, daz = spr->z+spriteheightofs(j, &height, 1);

                    if (pos->z > daz-height-flordist && pos->z < daz+ceildist)
                    {
                        int32_t bsz = (spr->clipdist << 2)+walldist;
                        if (diff.x < 0) bsz = -bsz;
                        addclipline(p1.x-bsz, p1.y-bsz, p1.x-bsz, p1.y+bsz, (int16_t)j+49152);
                        bsz = (spr->clipdist << 2)+walldist;
                        if (diff.y < 0) bsz = -bsz;
                        addclipline(p1.x+bsz, p1.y-bsz, p1.x-bsz, p1.y-bsz, (int16_t)j+49152);
                    }
                }
                break;

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                int32_t height, daz = spr->z+spriteheightofs(j, &height, 1);

                if (pos->z > daz-height-flordist && pos->z < spr->z+ceildist)
                {
                    vec2_t p2;

                    get_wallspr_points(spr, &p1.x, &p2.x, &p1.y, &p2.y);

                    if (clipinsideboxline(cent.x, cent.y, p1.x, p1.y, p2.x, p2.y, rad) != 0)
                    {
                        vec2_t v = { mulscale14(sintable[(spr->ang+256+512) & 2047], walldist),
                                     mulscale14(sintable[(spr->ang+256) & 2047], walldist) };

                        if ((p1.x-pos->x) * (p2.y-pos->y) >= (p2.x-pos->x) * (p1.y-pos->y))  // Front
                            addclipline(p1.x+v.x, p1.y+v.y, p2.x+v.y, p2.y-v.x, (int16_t)j+49152);
                        else
                        {
                            if ((cstat & 64) != 0)
                                continue;
                            addclipline(p2.x-v.x, p2.y-v.y, p1.x-v.y, p1.y+v.x, (int16_t)j+49152);
                        }

                        //Side blocker
                        if ((p2.x-p1.x) * (pos->x-p1.x)+(p2.y-p1.y) * (pos->y-p1.y) < 0)
                            addclipline(p1.x-v.y, p1.y+v.x, p1.x+v.x, p1.y+v.y, (int16_t)j+49152);
                        else if ((p1.x-p2.x) * (pos->x-p2.x)+(p1.y-p2.y) * (pos->y-p2.y) < 0)
                            addclipline(p2.x+v.y, p2.y-v.x, p2.x-v.x, p2.y-v.y, (int16_t)j+49152);
                    }
                }
                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
            {
                if (pos->z > spr->z-flordist && pos->z < spr->z+ceildist)
                {
                    if ((cstat&64) != 0)
                        if ((pos->z > spr->z) == ((cstat&8)==0))
                            continue;

                    rxi[0] = p1.x;
                    ryi[0] = p1.y;

                    get_floorspr_points((uspritetype const *) spr, 0, 0, &rxi[0], &rxi[1], &rxi[2], &rxi[3],
                        &ryi[0], &ryi[1], &ryi[2], &ryi[3]);

                    vec2_t v = { mulscale14(sintable[(spr->ang-256+512)&2047], walldist),
                                 mulscale14(sintable[(spr->ang-256)&2047], walldist) };

                    if ((rxi[0]-pos->x) * (ryi[1]-pos->y) < (rxi[1]-pos->x) * (ryi[0]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[1], ryi[1], rxi[0], ryi[0], rad) != 0)
                            addclipline(rxi[1]-v.y, ryi[1]+v.x, rxi[0]+v.x, ryi[0]+v.y, (int16_t)j+49152);
                    }
                    else if ((rxi[2]-pos->x) * (ryi[3]-pos->y) < (rxi[3]-pos->x) * (ryi[2]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[3], ryi[3], rxi[2], ryi[2], rad) != 0)
                            addclipline(rxi[3]+v.y, ryi[3]-v.x, rxi[2]-v.x, ryi[2]-v.y, (int16_t)j+49152);
                    }

                    if ((rxi[1]-pos->x) * (ryi[2]-pos->y) < (rxi[2]-pos->x) * (ryi[1]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[2], ryi[2], rxi[1], ryi[1], rad) != 0)
                            addclipline(rxi[2]-v.x, ryi[2]-v.y, rxi[1]-v.y, ryi[1]+v.x, (int16_t)j+49152);
                    }
                    else if ((rxi[3]-pos->x) * (ryi[0]-pos->y) < (rxi[0]-pos->x) * (ryi[3]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[0], ryi[0], rxi[3], ryi[3], rad) != 0)
                            addclipline(rxi[0]+v.x, ryi[0]+v.y, rxi[3]+v.y, ryi[3]-v.x, (int16_t)j+49152);
                    }
                }
                break;
            }
            }
        }
    } while (clipsectcnt < clipsectnum || clipspritecnt < clipspritenum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (curspr)
    {
        // restore original map
        engineSetClipMap(NULL, &origmapinfo);

        clipsectnum = origclipsectnum;
        Bmemcpy(clipsectorlist, origclipsectorlist, clipsectnum*sizeof(clipsectorlist[0]));
    }
#endif

    int32_t hitwalls[4], hitwall;
    int32_t clipReturn = 0;

    native_t cnt = clipmoveboxtracenum;

    do
    {
        vec2_t vec = goal;

        hitwall = raytrace(pos->x, pos->y, &vec.x, &vec.y);
        if (hitwall >= 0)
        {
            vec2_t const   clipr   = { clipit[hitwall].x2 - clipit[hitwall].x1, clipit[hitwall].y2 - clipit[hitwall].y1 };
            uint64_t const tempull = (int64_t)clipr.x * (int64_t)clipr.x + (int64_t)clipr.y * (int64_t)clipr.y;

            if (tempull > 0 && tempull < INT32_MAX)
            {
                int32_t const tempint2 = (int32_t) tempull;
                int32_t const tempint1 = (goal.x-vec.x)*clipr.x+(goal.y-vec.y)*clipr.y;
                int32_t const i = ((klabs(tempint1)>>11) < tempint2) ? divscale20(tempint1, tempint2) : 0;

                goal = { mulscale20(clipr.x, i)+vec.x, mulscale20(clipr.y, i)+vec.y };
            }

            int32_t const tempint1 = dmulscale6(clipr.x, move.x, clipr.y, move.y);

            for (int i=cnt+1, j=hitwalls[i]; i<=clipmoveboxtracenum; j=hitwalls[++i])
            {
                int32_t const tempint2 = dmulscale6(clipit[j].x2 - clipit[j].x1, move.x, clipit[j].y2 - clipit[j].y1, move.y);

                if ((tempint1^tempint2) < 0)
                {
                    updatesectorz(pos->x, pos->y, pos->z, sectnum);
                    return clipReturn;
                }
            }

            keepaway(&goal.x, &goal.y, hitwall);
            xvect = (goal.x-vec.x)<<14;
            yvect = (goal.y-vec.y)<<14;

            if (cnt == clipmoveboxtracenum)
                clipReturn = (uint16_t) clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }

        int const osectnum = *sectnum;
        updatesectorz(vec.x, vec.y, pos->z, sectnum);

        if (*sectnum == osectnum || editstatus || (*sectnum != -1 && !check_floor_curb(osectnum, *sectnum, flordist, pos->z, vec.x, vec.y)))
        {
            pos->x = vec.x;
            pos->y = vec.y;
            cnt--;
        }
        else
            *sectnum = osectnum;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

//    updatesectorz(pos->x, pos->y, pos->z, sectnum);
    return clipReturn;
}


//
// pushmove
//
int32_t pushmove(vec3_t *vect, int16_t *sectnum,
    int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    int32_t i, j, k, t, dx, dy, dax, day, daz;
    int32_t dir, bad, bad2;

    const int32_t dawalclipmask = (cliptype&65535);
    //    const int32_t dasprclipmask = (cliptype>>16);

    if (*sectnum < 0)
        return -1;

    k = 32;
    dir = 1;
    do
    {
        int32_t clipsectcnt;

        bad = 0;

        clipsectorlist[0] = *sectnum;
        clipsectcnt = 0; clipsectnum = 1;
        do
        {
            const uwalltype *wal;
            const usectortype *sec;
            int32_t startwall, endwall;
#if 0
            // Push FACE sprites
            for (i=headspritesect[clipsectorlist[clipsectcnt]]; i>=0; i=nextspritesect[i])
            {
                spr = &sprite[i];
                if (((spr->cstat&48) != 0) && ((spr->cstat&48) != 48)) continue;
                if ((spr->cstat&dasprclipmask) == 0) continue;

                dax = (vect->x)-spr->x; day = (vect->y)-spr->y;
                t = (spr->clipdist<<2)+walldist;
                if ((klabs(dax) < t) && (klabs(day) < t))
                {
                    daz = spr->z + spriteheightofs(i, &t, 1);
                    if (((vect->z) < daz+ceildist) && ((vect->z) > daz-t-flordist))
                    {
                        t = (spr->clipdist<<2)+walldist;

                        j = getangle(dax, day);
                        dx = (sintable[(j+512)&2047]>>11);
                        dy = (sintable[(j)&2047]>>11);
                        bad2 = 16;
                        do
                        {
                            vect->x = (vect->x) + dx; vect->y = (vect->y) + dy;
                            bad2--; if (bad2 == 0) break;
                        } while ((klabs((vect->x)-spr->x) < t) && (klabs((vect->y)-spr->y) < t));
                        bad = -1;
                        k--; if (k <= 0) return bad;
                        updatesector(vect->x, vect->y, sectnum);
                    }
                }
            }
#endif
            sec = (usectortype *)&sector[clipsectorlist[clipsectcnt]];
            if (dir > 0)
                startwall = sec->wallptr, endwall = startwall + sec->wallnum;
            else
                endwall = sec->wallptr, startwall = endwall + sec->wallnum;

            for (i=startwall, wal=(uwalltype *)&wall[startwall]; i!=endwall; i+=dir, wal+=dir)
                if (clipinsidebox((vec2_t *)vect, i, walldist-4) == 1)
                {
                    j = 0;
                    if (wal->nextsector < 0) j = 1;
                    if (wal->cstat&dawalclipmask) j = 1;
                    if (j == 0)
                    {
                        const usectortype *const sec2 = (usectortype *)&sector[wal->nextsector];
                        int32_t daz2;

                        //Find closest point on wall (dax, day) to (vect->x, vect->y)
                        dax = wall[wal->point2].x-wal->x;
                        day = wall[wal->point2].y-wal->y;
                        daz = dax*((vect->x)-wal->x) + day*((vect->y)-wal->y);
                        if (daz <= 0)
                            t = 0;
                        else
                        {
                            daz2 = dax*dax+day*day;
                            if (daz >= daz2) t = (1<<30); else t = divscale30(daz, daz2);
                        }
                        dax = wal->x + mulscale30(dax, t);
                        day = wal->y + mulscale30(day, t);


                        daz = getflorzofslope(clipsectorlist[clipsectcnt], dax, day);
                        daz2 = getflorzofslope(wal->nextsector, dax, day);
                        if ((daz2 < daz-(1<<8)) && ((sec2->floorstat&1) == 0))
                            if (vect->z >= daz2-(flordist-1)) j = 1;

                        daz = getceilzofslope(clipsectorlist[clipsectcnt], dax, day);
                        daz2 = getceilzofslope(wal->nextsector, dax, day);
                        if ((daz2 > daz+(1<<8)) && ((sec2->ceilingstat&1) == 0))
                            if (vect->z <= daz2+(ceildist-1)) j = 1;
                    }

                    if (j != 0)
                    {
                        j = getangle(wall[wal->point2].x-wal->x, wall[wal->point2].y-wal->y);
                        dx = (sintable[(j+1024)&2047]>>11);
                        dy = (sintable[(j+512)&2047]>>11);
                        bad2 = 16;
                        do
                        {
                            vect->x = (vect->x) + dx; vect->y = (vect->y) + dy;
                            bad2--; if (bad2 == 0) break;
                        } while (clipinsidebox((vec2_t *)vect, i, walldist-4) != 0);
                        bad = -1;
                        k--; if (k <= 0) return bad;
                        updatesector(vect->x, vect->y, sectnum);
                        if (*sectnum < 0) return -1;
                    }
                    else
                    {
                        for (j=clipsectnum-1; j>=0; j--)
                            if (wal->nextsector == clipsectorlist[j]) break;
                        if (j < 0) addclipsect(wal->nextsector);
                    }
                }

            clipsectcnt++;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return bad;
}

//
// getzrange
//
void getzrange(const vec3_t *pos, int16_t sectnum,
               int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit,
               int32_t walldist, uint32_t cliptype)
{
    if (sectnum < 0)
    {
        *ceilz = INT32_MIN; *ceilhit = -1;
        *florz = INT32_MAX; *florhit = -1;
        return;
    }

    int32_t clipsectcnt = 0;

#ifdef YAX_ENABLE
    // YAX round, -1:center, 0:ceiling, 1:floor
    int32_t mcf=-1;
#endif

    uspritetype *curspr=NULL;  // non-NULL when handling sprite with sector-like clipping
    int32_t curidx=-1, clipspritecnt = 0;

    //Extra walldist for sprites on sector lines
    const int32_t extradist = walldist+MAXCLIPDIST+1;
    const int32_t xmin = pos->x-extradist, ymin = pos->y-extradist;
    const int32_t xmax = pos->x+extradist, ymax = pos->y+extradist;

    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype>>16);

    getzsofslope(sectnum,pos->x,pos->y,ceilz,florz);
    *ceilhit = sectnum+16384; *florhit = sectnum+16384;

#ifdef YAX_ENABLE
    origclipsectorlist[0] = sectnum;
    origclipsectnum = 1;
#endif
    clipsectorlist[0] = sectnum;
    clipsectnum = 1;
    clipspritenum = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (0)
    {
beginagain:
        // replace sector and wall with clip map
        engineSetClipMap(&origmapinfo, &clipmapinfo);
        clipsectcnt = clipsectnum;  // should be a nop, "safety"...
    }
#endif

#ifdef YAX_ENABLE
restart_grand:
#endif
    do  //Collect sectors inside your square first
    {
#ifdef HAVE_CLIPSHAPE_FEATURE
        if (clipsectcnt>=clipsectnum)
        {
            // one set of clip-sprite sectors completed, prepare the next

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            curidx = clipshape_idx_for_sprite(curspr, curidx);

            if (curidx < 0)
            {
                // didn't find matching clipping sectors for sprite
                clipspritecnt++;
                continue;
            }

            clipsprite_initindex(curidx, curspr, &clipsectcnt, pos);

            for (bssize_t i=0; i<clipsectnum; i++)
            {
                int const k = clipsectorlist[i];

                if (k==sectq[clipinfo[curidx].qend])
                    continue;

                int32_t daz, daz2;
                getzsofslope(k,pos->x,pos->y,&daz,&daz2);
                int32_t fz, cz;
                getzsofslope(sectq[clipinfo[curidx].qend],pos->x,pos->y,&cz,&fz);
                const int hitwhat = (curspr-(uspritetype *)sprite)+49152;

                if ((sector[k].ceilingstat&1)==0)
                {
                    if (pos->z < cz && cz < *florz) { *florz = cz; *florhit = hitwhat; }
                    if (pos->z > daz && daz > *ceilz) { *ceilz = daz; *ceilhit = hitwhat; }
                }
                if ((sector[k].floorstat&1)==0)
                {
                    if (pos->z < daz2 && daz2 < *florz) { *florz = daz2; *florhit = hitwhat; }
                    if (pos->z > fz && fz > *ceilz) { *ceilz = fz; *ceilhit = hitwhat; }
                }
            }
        }
#endif
        ////////// Walls //////////

        const sectortype *const startsec = &sector[clipsectorlist[clipsectcnt]];
        const int startwall = startsec->wallptr;
        const int endwall = startwall + startsec->wallnum;

        for (bssize_t j=startwall; j<endwall; j++)
        {
            const int k = wall[j].nextsector;

            if (k >= 0)
            {
                vec2_t const v1 = *(vec2_t *)&wall[j];
                vec2_t const v2 = *(vec2_t *)&wall[wall[j].point2];

                if ((v1.x < xmin && (v2.x < xmin)) || (v1.x > xmax && v2.x > xmax) ||
                    (v1.y < ymin && (v2.y < ymin)) || (v1.y > ymax && v2.y > ymax))
                    continue;

                vec2_t const d = { v2.x-v1.x, v2.y-v1.y };
                if (d.x*(pos->y-v1.y) < (pos->x-v1.x)*d.y) continue; //back

                vec2_t da = { (d.x > 0) ? d.x*(ymin-v1.y) : d.x*(ymax-v1.y),
                              (d.y > 0) ? d.y*(xmax-v1.x) : d.y*(xmin-v1.x) };

                if (da.x >= da.y)
                    continue;

                if (wall[j].cstat&dawalclipmask) continue;  // XXX?
                const sectortype *const sec = &sector[k];

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (curspr)
                {
                    if (k==sectq[clipinfo[curidx].qend])
                        continue;
                    if ((sec->ceilingstat&1) && (sec->floorstat&1))
                        continue;
                }
                else
#endif
                if (editstatus == 0)
                {
                    if (((sec->ceilingstat&1) == 0) && (pos->z <= sec->ceilingz+(3<<8))) continue;
                    if (((sec->floorstat&1) == 0) && (pos->z >= sec->floorz-(3<<8))) continue;
                }

                int i;
                for (i=clipsectnum-1; i>=0; --i)
                    if (clipsectorlist[i] == k) break;

                if (i < 0) addclipsect(k);

                if (((v1.x < xmin + MAXCLIPDIST) && (v2.x < xmin + MAXCLIPDIST)) ||
                    ((v1.x > xmax - MAXCLIPDIST) && (v2.x > xmax - MAXCLIPDIST)) ||
                    ((v1.y < ymin + MAXCLIPDIST) && (v2.y < ymin + MAXCLIPDIST)) ||
                    ((v1.y > ymax - MAXCLIPDIST) && (v2.y > ymax - MAXCLIPDIST)))
                    continue;

                if (d.x > 0) da.x += d.x*MAXCLIPDIST; else da.x -= d.x*MAXCLIPDIST;
                if (d.y > 0) da.y -= d.y*MAXCLIPDIST; else da.y += d.y*MAXCLIPDIST;
                if (da.x >= da.y)
                    continue;
#ifdef YAX_ENABLE
                if (mcf==-1 && curspr==NULL)
                    origclipsectorlist[origclipsectnum++] = k;
#endif
                //It actually got here, through all the continue's!!!
                int32_t daz, daz2;
                getzsofslope(k, pos->x,pos->y, &daz,&daz2);

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (curspr)
                {
                    int32_t fz,cz, hitwhat=(curspr-(uspritetype *)sprite)+49152;
                    getzsofslope(sectq[clipinfo[curidx].qend],pos->x,pos->y,&cz,&fz);

                    if ((sec->ceilingstat&1)==0)
                    {
                        if (pos->z < cz && cz < *florz) { *florz = cz; *florhit = hitwhat; }
                        if (pos->z > daz && daz > *ceilz) { *ceilz = daz; *ceilhit = hitwhat; }
                    }
                    if ((sec->floorstat&1)==0)
                    {
                        if (pos->z < daz2 && daz2 < *florz) { *florz = daz2; *florhit = hitwhat; }
                        if (pos->z > fz && fz > *ceilz) { *ceilz = fz; *ceilhit = hitwhat; }
                    }
                }
                else
#endif
                {
#ifdef YAX_ENABLE
                    int16_t cb, fb;
                    yax_getbunches(k, &cb, &fb);
#endif
                    if (daz > *ceilz)
#ifdef YAX_ENABLE
                        if (mcf!=YAX_FLOOR && cb < 0)
#endif
                        *ceilz = daz, *ceilhit = k+16384;

                    if (daz2 < *florz)
#ifdef YAX_ENABLE
                        if (mcf!=YAX_CEILING && fb < 0)
#endif
                        *florz = daz2, *florhit = k+16384;
                }
            }
        }
        clipsectcnt++;
    }
    while (clipsectcnt < clipsectnum || clipspritecnt < clipspritenum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (curspr)
    {
        engineSetClipMap(NULL, &origmapinfo);  // restore original map
        clipsectnum = clipspritenum = 0;  // skip the next for loop and check afterwards
    }
#endif

    ////////// Sprites //////////

    if (dasprclipmask)
    for (bssize_t i=0; i<clipsectnum; i++)
    {
        for (bssize_t j=headspritesect[clipsectorlist[i]]; j>=0; j=nextspritesect[j])
        {
            const int32_t cstat = sprite[j].cstat;
            int32_t daz, daz2;

            if (cstat&dasprclipmask)
            {
                int32_t clipyou = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (clipsprite_try((uspritetype *)&sprite[j], xmin,ymin, xmax,ymax))
                    continue;
#endif
                vec2_t v1 = *(vec2_t *)&sprite[j];

                switch (cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
                {
                    case CSTAT_SPRITE_ALIGNMENT_FACING:
                    {
                        int32_t k = walldist+(sprite[j].clipdist<<2)+1;
                        if ((klabs(v1.x-pos->x) <= k) && (klabs(v1.y-pos->y) <= k))
                        {
                            daz = sprite[j].z + spriteheightofs(j, &k, 1);
                            daz2 = daz - k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_WALL:
                    {
                        vec2_t v2;
                        get_wallspr_points((uspritetype *)&sprite[j], &v1.x, &v2.x, &v1.y, &v2.y);

                        if (clipinsideboxline(pos->x,pos->y,v1.x,v1.y,v2.x,v2.y,walldist+1) != 0)
                        {
                            int32_t k;
                            daz = sprite[j].z + spriteheightofs(j, &k, 1);
                            daz2 = daz-k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_FLOOR:
                    {
                        daz = sprite[j].z; daz2 = daz;

                        if ((cstat&64) != 0 && (pos->z > daz) == ((cstat&8)==0))
                            continue;

                        vec2_t v2, v3, v4;
                        get_floorspr_points((uspritetype const *) &sprite[j], pos->x, pos->y, &v1.x, &v2.x, &v3.x, &v4.x,
                                            &v1.y, &v2.y, &v3.y, &v4.y);

                        vec2_t const da = { mulscale14(sintable[(sprite[j].ang - 256 + 512) & 2047], walldist + 4),
                                            mulscale14(sintable[(sprite[j].ang - 256) & 2047], walldist + 4) };

                        v1.x += da.x; v2.x -= da.y; v3.x -= da.x; v4.x += da.y;
                        v1.y += da.y; v2.y += da.x; v3.y -= da.y; v4.y -= da.x;

                        clipyou = get_floorspr_clipyou(v1.x, v2.x, v3.x, v4.x, v1.y, v2.y, v3.y, v4.y);
                        break;
                    }
                }

                if (clipyou != 0)
                {
                    if ((pos->z > daz) && (daz > *ceilz
#ifdef YAX_ENABLE
                                           || (daz == *ceilz && yax_getbunch(clipsectorlist[i], YAX_CEILING)>=0)
#endif
                            ))
                    {
                        *ceilz = daz;
                        *ceilhit = j+49152;
                    }

                    if ((pos->z < daz2) && (daz2 < *florz
#ifdef YAX_ENABLE
                                            // can have a floor-sprite lying directly on the floor!
                                            || (daz2 == *florz && yax_getbunch(clipsectorlist[i], YAX_FLOOR)>=0)
#endif
                            ))
                    {
                        *florz = daz2;
                        *florhit = j+49152;
                    }
                }
            }
        }
    }

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (clipspritenum>0)
        goto beginagain;
#endif

#ifdef YAX_ENABLE
    if (numyaxbunches > 0)
    {
        int const dasecclipmask = yax_waltosecmask(dawalclipmask);
        int16_t cb, fb;

        yax_getbunches(sectnum, &cb, &fb);

        mcf++;
        clipsectcnt = 0; clipsectnum = 0;

        int didchange = 0;
        if (cb>=0 && mcf==0 && *ceilhit==sectnum+16384)
        {
            int i;
            for (i=0; i<origclipsectnum; i++)
            {
                int const j = origclipsectorlist[i];
                if (yax_getbunch(j, YAX_CEILING) >= 0)
                    if (sector[j].ceilingstat&dasecclipmask)
                        break;
            }

            if (i==origclipsectnum)
                for (i=0; i<origclipsectnum; i++)
                {
                    cb = yax_getbunch(origclipsectorlist[i], YAX_CEILING);
                    if (cb < 0)
                        continue;

                    for (bssize_t SECTORS_OF_BUNCH(cb,YAX_FLOOR, j))
                        if (inside(pos->x,pos->y, j)==1)
                        {
                            addclipsect(j);
                            int const daz = getceilzofslope(j, pos->x,pos->y);
                            if (!didchange || daz > *ceilz)
                                didchange=1, *ceilhit = j+16384, *ceilz = daz;
                        }
                }

            if (clipsectnum==0)
                mcf++;
        }
        else if (mcf==0)
            mcf++;

        didchange = 0;
        if (fb>=0 && mcf==1 && *florhit==sectnum+16384)
        {
            int i=0;
            for (; i<origclipsectnum; i++)
            {
                int const j = origclipsectorlist[i];
                if (yax_getbunch(j, YAX_FLOOR) >= 0)
                    if (sector[j].floorstat&dasecclipmask)
                        break;
            }

            // (almost) same as above, but with floors...
            if (i==origclipsectnum)
                for (i=0; i<origclipsectnum; i++)
                {
                    fb = yax_getbunch(origclipsectorlist[i], YAX_FLOOR);
                    if (fb < 0)
                        continue;

                    for (bssize_t SECTORS_OF_BUNCH(fb, YAX_CEILING, j))
                        if (inside(pos->x,pos->y, j)==1)
                        {
                            addclipsect(j);
                            int const daz = getflorzofslope(j, pos->x,pos->y);
                            if (!didchange || daz < *florz)
                                didchange=1, *florhit = j+16384, *florz = daz;
                        }
                }
        }

        if (clipsectnum > 0)
        {
            // sector-like sprite re-init:
            curidx = -1;
            curspr = NULL;
            clipspritecnt = 0; clipspritenum = 0;

            goto restart_grand;
        }
    }
#endif
}


// intp: point of currently best (closest) intersection
int32_t try_facespr_intersect(uspritetype const * const spr, const vec3_t *refpos,
                                     int32_t vx, int32_t vy, int32_t vz,
                                     vec3_t *intp, int32_t strictly_smaller_than_p)
{
    const int32_t x1=spr->x, y1=spr->y;
    const int32_t xs=refpos->x, ys=refpos->y;

    const int32_t topt = vx*(x1-xs) + vy*(y1-ys);
    if (topt > 0)
    {
        const int32_t bot = vx*vx + vy*vy;
        if (bot != 0)
        {
            int32_t i;
            const int32_t intz = refpos->z + scale(vz,topt,bot);
            const int32_t z1 = spr->z + spriteheightofsptr(spr, &i, 1);

            if (intz >= z1-i && intz <= z1)
            {
                const int32_t topu = vx*(y1-ys) - vy*(x1-xs);

                const int32_t offx = scale(vx,topu,bot);
                const int32_t offy = scale(vy,topu,bot);
                const int32_t dist = offx*offx + offy*offy;

                i = tilesiz[spr->picnum].x*spr->xrepeat;
                if (dist <= mulscale7(i,i))
                {
                    const int32_t intx = xs + scale(vx,topt,bot);
                    const int32_t inty = ys + scale(vy,topt,bot);

                    if (klabs(intx-xs)+klabs(inty-ys) + strictly_smaller_than_p
                            <= klabs(intp->x-xs)+klabs(intp->y-ys))
                    {
                        intp->x = intx;
                        intp->y = inty;
                        intp->z = intz;
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

static inline void hit_set(hitdata_t *hit, int32_t sectnum, int32_t wallnum, int32_t spritenum,
                           int32_t x, int32_t y, int32_t z)
{
    hit->sect = sectnum;
    hit->wall = wallnum;
    hit->sprite = spritenum;
    hit->pos.x = x;
    hit->pos.y = y;
    hit->pos.z = z;
}

static int32_t hitscan_hitsectcf=-1;

// stat, heinum, z: either ceiling- or floor-
// how: -1: behave like ceiling, 1: behave like floor
static int32_t hitscan_trysector(const vec3_t *sv, const usectortype *sec, hitdata_t *hit,
                                 int32_t vx, int32_t vy, int32_t vz,
                                 uint16_t stat, int16_t heinum, int32_t z, int32_t how, const intptr_t *tmp)
{
    int32_t x1 = INT32_MAX, y1, z1;
    int32_t i;

    if (stat&2)
    {
        auto const wal  = (uwalltype *)&wall[sec->wallptr];
        auto const wal2 = (uwalltype *)&wall[wal->point2];
        int32_t j, dax=wal2->x-wal->x, day=wal2->y-wal->y;

        i = nsqrtasm(uhypsq(dax,day)); if (i == 0) return 1; //continue;
        i = divscale15(heinum,i);
        dax *= i; day *= i;

        j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
        if (j != 0)
        {
            i = ((z - sv->z)<<8)+dmulscale15(dax,sv->y-wal->y,-day,sv->x-wal->x);
            if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
            {
                i = divscale30(i,j);
                x1 = sv->x + mulscale30(vx,i);
                y1 = sv->y + mulscale30(vy,i);
                z1 = sv->z + mulscale30(vz,i);
            }
        }
    }
    else if ((how*vz > 0) && (how*sv->z <= how*z))
    {
        z1 = z; i = z1-sv->z;
        if ((klabs(i)>>1) < vz*how)
        {
            i = divscale30(i,vz);
            x1 = sv->x + mulscale30(vx,i);
            y1 = sv->y + mulscale30(vy,i);
        }
    }

    if ((x1 != INT32_MAX) && (klabs(x1-sv->x)+klabs(y1-sv->y) < klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y)))
    {
        if (tmp==NULL)
        {
            if (inside(x1,y1,sec-(usectortype *)sector) == 1)
            {
                hit_set(hit, sec-(usectortype *)sector, -1, -1, x1, y1, z1);
                hitscan_hitsectcf = (how+1)>>1;
            }
        }
        else
        {
            const int32_t curidx=(int32_t)tmp[0];
            const uspritetype *const curspr=(uspritetype *)tmp[1];
            const int32_t thislastsec = tmp[2];

            if (!thislastsec)
            {
                if (inside(x1,y1,sec-(usectortype *)sector) == 1)
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, x1, y1, z1);
            }
#ifdef HAVE_CLIPSHAPE_FEATURE
            else
            {
                for (i=clipinfo[curidx].qbeg; i<clipinfo[curidx].qend; i++)
                {
                    if (inside(x1,y1,sectq[i]) == 1)
                    {
                        hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, x1, y1, z1);
                        break;
                    }
                }
            }
#endif
        }
    }

    return 0;
}

//
// hitscan
//
int32_t hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                hitdata_t *hit, uint32_t cliptype)
{
    int32_t x1, y1=0, z1=0, x2, y2, intx, inty, intz;
    int32_t i, k, daz;
    int16_t tempshortcnt, tempshortnum;

    uspritetype *curspr = NULL;
    int32_t clipspritecnt, curidx=-1;
    // tmp: { (int32_t)curidx, (spritetype *)curspr, (!=0 if outer sector) }
    intptr_t tmp[3], *tmpptr=NULL;
#ifdef YAX_ENABLE
    vec3_t newsv;
    int32_t oldhitsect = -1, oldhitsect2 = -2;
#endif
    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype>>16);

    hit->sect = -1; hit->wall = -1; hit->sprite = -1;
    if (sectnum < 0)
        return -1;

#ifdef YAX_ENABLE
restart_grand:
#endif
    *(vec2_t *)&hit->pos = hitscangoal;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0; tempshortnum = 1;
    clipspritecnt = clipspritenum = 0;
    do
    {
        int32_t dasector, z, startwall, endwall;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (tempshortcnt >= tempshortnum)
        {
            // one bunch of sectors completed, prepare the next
            if (!curspr)
                engineSetClipMap(&origmapinfo, &clipmapinfo);  // replace sector and wall with clip map

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            curidx = clipshape_idx_for_sprite(curspr, curidx);

            if (curidx < 0)
            {
                clipspritecnt++;
                continue;
            }

            tmp[0] = (intptr_t)curidx;
            tmp[1] = (intptr_t)curspr;
            tmpptr = tmp;

            clipsprite_initindex(curidx, curspr, &i, sv);  // &i is dummy
            tempshortnum = (int16_t)clipsectnum;
            tempshortcnt = 0;
        }
#endif
        dasector = clipsectorlist[tempshortcnt];
        auto const * sec = (usectortype *)&sector[dasector];

        i = 1;
#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
        {
            if (dasector == sectq[clipinfo[curidx].qend])
            {
                i = -1;
                tmp[2] = 1;
            }
            else tmp[2] = 0;
        }
#endif
        if (hitscan_trysector(sv, sec, hit, vx,vy,vz, sec->ceilingstat, sec->ceilingheinum, sec->ceilingz, -i, tmpptr))
            continue;
        if (hitscan_trysector(sv, sec, hit, vx,vy,vz, sec->floorstat, sec->floorheinum, sec->floorz, i, tmpptr))
            continue;

        ////////// Walls //////////

        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (z=startwall; z<endwall; z++)
        {
            auto const wal  = (uwalltype *)&wall[z];
            auto const wal2 = (uwalltype *)&wall[wal->point2];

            int const  nextsector = wal->nextsector;

            if (curspr && nextsector<0) continue;

            int32_t daz2, zz;

            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            if ((coord_t)(x1-sv->x)*(y2-sv->y) < (coord_t)(x2-sv->x)*(y1-sv->y)) continue;
            if (rintersect(sv->x,sv->y,sv->z, vx,vy,vz, x1,y1, x2,y2, &intx,&inty,&intz) == -1) continue;

            if (klabs(intx-sv->x)+klabs(inty-sv->y) >= klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                continue;

            if (!curspr)
            {
                if ((nextsector < 0) || (wal->cstat&dawalclipmask))
                {
                    hit_set(hit, dasector, z, -1, intx, inty, intz);
                    continue;
                }

                getzsofslope(nextsector,intx,inty,&daz,&daz2);
                if (intz <= daz || intz >= daz2)
                {
                    hit_set(hit, dasector, z, -1, intx, inty, intz);
                    continue;
                }
            }
#ifdef HAVE_CLIPSHAPE_FEATURE
            else
            {
                int32_t cz,fz;

                if (wal->cstat&dawalclipmask)
                {
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, intx, inty, intz);
                    continue;
                }

                getzsofslope(nextsector,intx,inty,&daz,&daz2);
                getzsofslope(sectq[clipinfo[curidx].qend],intx,inty,&cz,&fz);
                // ceil   cz daz daz2 fz   floor
                if ((cz <= intz && intz <= daz) || (daz2 <= intz && intz <= fz))
                {
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, intx, inty, intz);
                    continue;
                }
            }
#endif
            for (zz=tempshortnum-1; zz>=0; zz--)
                if (clipsectorlist[zz] == nextsector) break;
            if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
            continue;
#endif
        for (z=headspritesect[dasector]; z>=0; z=nextspritesect[z])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[z];
            const int32_t cstat = spr->cstat;
#ifdef USE_OPENGL
            if (!hitallsprites)
#endif
                if ((cstat&dasprclipmask) == 0)
                    continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
            // try and see whether this sprite's picnum has sector-like clipping data
            i = pictoidx[spr->picnum];
            // handle sector-like floor sprites separately
            while (i>=0 && (spr->cstat&32) != (clipmapinfo.sector[sectq[clipinfo[i].qbeg]].CM_CSTAT&32))
                i = clipinfo[i].next;
            if (i>=0 && clipspritenum<MAXCLIPNUM)
            {
                clipspritelist[clipspritenum++] = z;
                continue;
            }
#endif
            x1 = spr->x; y1 = spr->y; z1 = spr->z;
            switch (cstat&CSTAT_SPRITE_ALIGNMENT)
            {
            case 0:
            {
                if (try_facespr_intersect(spr, sv, vx, vy, vz, &hit->pos, 0))
                {
                    hit->sect = dasector;
                    hit->wall = -1;
                    hit->sprite = z;
                }

                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                int32_t ucoefup16;
                int32_t tilenum = spr->picnum;

                get_wallspr_points(spr, &x1, &x2, &y1, &y2);

                if ((cstat&64) != 0)   //back side of 1-way sprite
                    if ((coord_t)(x1-sv->x)*(y2-sv->y) < (coord_t)(x2-sv->x)*(y1-sv->y)) continue;

                ucoefup16 = rintersect(sv->x,sv->y,sv->z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz);
                if (ucoefup16 == -1) continue;

                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                    continue;

                daz = spr->z + spriteheightofs(z, &k, 1);
                if (intz > daz-k && intz < daz)
                {
                    if (picanm[tilenum].sf&PICANM_TEXHITSCAN_BIT)
                    {
                        DO_TILE_ANIM(tilenum, 0);

                        if (!waloff[tilenum])
                            tileLoad(tilenum);

                        if (waloff[tilenum])
                        {
                            // daz-intz > 0 && daz-intz < k
                            int32_t xtex = mulscale16(ucoefup16, tilesiz[tilenum].x);
                            int32_t vcoefup16 = 65536-divscale16(daz-intz, k);
                            int32_t ytex = mulscale16(vcoefup16, tilesiz[tilenum].y);

                            const char *texel = (char *)(waloff[tilenum] + tilesiz[tilenum].y*xtex + ytex);
                            if (*texel == 255)
                                continue;
                        }
                    }

                    hit_set(hit, dasector, -1, z, intx, inty, intz);
                }
                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
            {
                int32_t x3, y3, x4, y4, zz;

                if (vz == 0) continue;
                intz = z1;
                if (((intz-sv->z)^vz) < 0) continue;
                if ((cstat&64) != 0)
                    if ((sv->z > intz) == ((cstat&8)==0)) continue;
#if 1
                // Abyss crash prevention code ((intz-sv->z)*zx overflowing a 8-bit word)
                // PK: the reason for the crash is not the overflowing (even if it IS a problem;
                // signed overflow is undefined behavior in C), but rather the idiv trap when
                // the resulting quotient doesn't fit into a *signed* 32-bit integer.
                zz = (uint32_t)(intz-sv->z) * vx;
                intx = sv->x+scale(zz,1,vz);
                zz = (uint32_t)(intz-sv->z) * vy;
                inty = sv->y+scale(zz,1,vz);
#else
                intx = sv->x+scale(intz-sv->z,vx,vz);
                inty = sv->y+scale(intz-sv->z,vy,vz);
#endif
                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                    continue;

                get_floorspr_points((uspritetype const *)spr, intx, inty, &x1, &x2, &x3, &x4,
                                    &y1, &y2, &y3, &y4);

                if (get_floorspr_clipyou(x1, x2, x3, x4, y1, y2, y3, y4))
                {
                    hit_set(hit, dasector, -1, z, intx, inty, intz);
                }

                break;
            }
            }
        }
    }
    while (++tempshortcnt < tempshortnum || clipspritecnt < clipspritenum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (curspr)
        engineSetClipMap(NULL, &origmapinfo);
#endif

#ifdef YAX_ENABLE
    if (numyaxbunches == 0 || editstatus)
        return 0;

    if (hit->sprite==-1 && hit->wall==-1 && hit->sect!=oldhitsect
        && hit->sect != oldhitsect2)  // 'ping-pong' infloop protection
    {
        if (hit->sect == -1 && oldhitsect >= 0)
        {
            // this is bad: we didn't hit anything after going through a ceiling/floor
            Bmemcpy(&hit->pos, &newsv, sizeof(vec3_t));
            hit->sect = oldhitsect;

            return 0;
        }

        // 1st, 2nd, ... ceil/floor hit
        // hit->sect is >=0 because if oldhitsect's init and check above
        if (SECTORFLD(hit->sect,stat, hitscan_hitsectcf)&yax_waltosecmask(dawalclipmask))
            return 0;

        i = yax_getneighborsect(hit->pos.x, hit->pos.y, hit->sect, hitscan_hitsectcf);
        if (i >= 0)
        {
            Bmemcpy(&newsv, &hit->pos, sizeof(vec3_t));
            sectnum = i;
            sv = &newsv;

            oldhitsect2 = oldhitsect;
            oldhitsect = hit->sect;
            hit->sect = -1;

            // sector-like sprite re-init:
            curspr = 0;
            curidx = -1;
            tmpptr = NULL;

            goto restart_grand;
        }
    }
#endif

    return 0;
}

