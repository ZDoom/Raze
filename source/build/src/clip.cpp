#include "build.h"
#include "baselayer.h"
#include "engine_priv.h"

int16_t clipnum;
linetype clipit[MAXCLIPNUM];
int32_t clipsectnum, origclipsectnum, clipspritenum;
int16_t clipsectorlist[MAXCLIPNUM], origclipsectorlist[MAXCLIPNUM];
#ifdef HAVE_CLIPSHAPE_FEATURE
int16_t clipspritelist[MAXCLIPNUM];  // sector-like sprite clipping
#endif
static int16_t clipobjectval[MAXCLIPNUM];

////// sector-like clipping for sprites //////
#ifdef HAVE_CLIPSHAPE_FEATURE
void mapinfo_set(mapinfo_t *bak, mapinfo_t *newmap)
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


void clipmapinfo_init()
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
int32_t clipmapinfo_load(void)
{
    int32_t i, k, w;

    int32_t lwcp = 0;
    int32_t fi;

    int32_t *fisec = NULL;
    int32_t *fispr = NULL;

    int32_t ournumsectors=0, ournumwalls=0, ournumsprites=0;

    clipmapinfo_init();

    loadsector = (usectortype *) Xmalloc(MAXSECTORS * sizeof(sectortype));
    loadwall = (uwalltype *) Xmalloc(MAXWALLS * sizeof(walltype));
    loadsprite = (uspritetype *) Xmalloc(MAXSPRITES * sizeof(spritetype));

    if (g_clipMapFilesNum)
        fisec = (int32_t *) Xcalloc(g_clipMapFilesNum, sizeof(int32_t));
    if (g_clipMapFilesNum)
        fispr = (int32_t *) Xcalloc(g_clipMapFilesNum, sizeof(int32_t));

    quickloadboard = 1;
    for (fi = 0; fi < g_clipMapFilesNum; ++fi)
    {
        int16_t ang, cs;
        vec3_t tmppos;

        fisec[fi] = ournumsectors;
        fispr[fi] = ournumsprites;

        i = loadboard(g_clipMapFiles[fi], 8, &tmppos, &ang, &cs);
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
        clipmapinfo_init();

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
                    clipmapinfo_init();

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
                                clipmapinfo_init();

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
                            clipmapinfo_init();

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
                clipmapinfo_init();

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
    int const        r   = walldist << 1;
    uwalltype const *wal = (uwalltype *)&wall[wallnum];
    vec2_t const     v1  = { wal->x + walldist - vect->x, wal->y + walldist - vect->y };
    wal                  = (uwalltype *)&wall[wal->point2];
    vec2_t v2            = { wal->x + walldist - vect->x, wal->y + walldist - vect->y };

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

        if ((spr->cstat&48)!=32)  // face/wall sprite
        {
            int32_t const tempint1 = clipmapinfo.sector[k].CM_XREPEAT;
            maxcorrection = divideu32_noinline(maxcorrection * (int32_t) spr->xrepeat, tempint1);
        }
        else  // floor sprite
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

    if ((curspr->cstat&48)!=32)  // face/wall sprite
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
        const int32_t j = sectq[k];
        usectortype *const sec = (usectortype *)&sector[j];
        const int32_t startwall = sec->wallptr, endwall = startwall+sec->wallnum;

        int32_t w;
        uwalltype *wal;

        sec->floorz = daz + mulscale22(scalez, CM_FLOORZ(j));
        sec->ceilingz = daz + mulscale22(scalez, CM_CEILINGZ(j));
        //initprintf("sec %d: f=%d, c=%d\n", j, sec->floorz, sec->ceilingz);

        for (w=startwall, wal=(uwalltype *)&wall[startwall]; w<endwall; w++, wal++)
        {
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
            clipsectorlist[clipsectnum++] = j;
    }

    // add outer sector if not inside inner ones
    if (clipsectnum==0)
        clipsectorlist[clipsectnum++] = sectq[k-1];

    return flipmul;
}

#endif

static int32_t clipmove_warned=0;

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
        clipmove_warned = 1;
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
    usectortype const * const sec2 = (usectortype *)&sector[nextsect];
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

    while (1)
    {
        if (dx*(*y-y1) > (*x-x1)*dy)
            return;

        if (first == 0)
            *x += ox;
        else
            *y += oy;

        first ^= 1;
    }
}

//
// clipmove
//
int32_t clipmove(vec3_t *pos, int16_t *sectnum,
    int32_t xvect, int32_t yvect,
    int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return 0;

    int32_t i, j, k, tempint1, tempint2;
    int32_t x1, y1, x2, y2;
    int32_t dax, day;
    int32_t retval=0;

    uspritetype const * curspr=NULL;  // non-NULL when handling sprite with sector-like clipping
    int32_t curidx=-1, clipsectcnt, clipspritecnt;

    const int32_t dawalclipmask = (cliptype&65535);        //CLIPMASK0 = 0x00010001
    const int32_t dasprclipmask = (cliptype>>16);          //CLIPMASK1 = 0x01000040

    const int32_t oxvect=xvect, oyvect=yvect;

    int32_t goalx = pos->x + (xvect>>14);
    int32_t goaly = pos->y + (yvect>>14);
    const int32_t cx = (pos->x+goalx)>>1;
    const int32_t cy = (pos->y+goaly)>>1;

    //Extra walldist for sprites on sector lines
    const int32_t gx=goalx-(pos->x), gy=goaly-(pos->y);
    const int32_t rad = nsqrtasm(uhypsq(gx, gy)) + MAXCLIPDIST+walldist + 8;
    const int32_t xmin = cx-rad, ymin = cy-rad;
    const int32_t xmax = cx+rad, ymax = cy+rad;


    clipmove_warned = 0;
    clipnum = 0;

    clipsectorlist[0] = (*sectnum);
    clipsectcnt = 0; clipsectnum = 1;
    clipspritecnt = 0; clipspritenum = 0;
    do
    {
        const uwalltype *wal;
        const usectortype *sec;
        int32_t dasect, startwall, endwall;

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
                mapinfo_set(&origmapinfo, &clipmapinfo);
            }

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            curidx = clipshape_idx_for_sprite(curspr, curidx);

            if (curidx < 0)
            {
                clipspritecnt++;
                continue;
            }

            clipsprite_initindex(curidx, curspr, &clipsectcnt, pos);
        }
#endif

        dasect = clipsectorlist[clipsectcnt++];
        //if (curspr)
        //    initprintf("sprite %d/%d: sect %d/%d (%d)\n", clipspritecnt,clipspritenum, clipsectcnt,clipsectnum,dasect);

        ////////// Walls //////////

        sec = (usectortype *)&sector[dasect];
        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (j=startwall, wal=(uwalltype *)&wall[startwall]; j<endwall; j++, wal++)
        {
            int32_t clipyou = 0, dx, dy;
            const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];

            if ((wal->x < xmin && wal2->x < xmin) || (wal->x > xmax && wal2->x > xmax) ||
                (wal->y < ymin && wal2->y < ymin) || (wal->y > ymax && wal2->y > ymax))
                continue;

            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            dx = x2-x1; dy = y2-y1;
            if (dx*((pos->y)-y1) < ((pos->x)-x1)*dy) continue;  //If wall's not facing you

            if (dx > 0) dax = dx*(ymin-y1); else dax = dx*(ymax-y1);
            if (dy > 0) day = dy*(xmax-x1); else day = dy*(xmin-x1);
            if (dax >= day) continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
            if (curspr)
            {
                if (wal->nextsector>=0)
                {
                    const usectortype *sec2 = (usectortype *)&sector[wal->nextsector];

                    clipmove_tweak_pos(pos, gx, gy, x1, y1, x2, y2, &dax, &day);

#define CLIPMV_SPR_F_DAZ2 getflorzofslope(wal->nextsector, dax,day)
#define CLIPMV_SPR_F_BASEZ getflorzofslope(sectq[clipinfo[curidx].qend], dax,day)

                    if ((sec2->floorstat&1) == 0)
                        if (CLIPMV_SPR_F_DAZ2-(flordist-1) <= pos->z)
                            if (pos->z <= CLIPMV_SPR_F_BASEZ+(flordist-1))
                                clipyou = 1;

                    if (clipyou == 0)
                    {
#define CLIPMV_SPR_C_DAZ2 getceilzofslope(wal->nextsector, dax,day)
#define CLIPMV_SPR_C_BASEZ getceilzofslope(sectq[clipinfo[curidx].qend], dax,day)

                        if ((sec2->ceilingstat&1) == 0)
                            if (CLIPMV_SPR_C_BASEZ-(ceildist-1) <= pos->z)
                                if (pos->z <= CLIPMV_SPR_C_DAZ2+(ceildist-1))
                                    clipyou = 1;
                    }
                }
            }
            else
#endif
                if (wal->nextsector < 0 || (wal->cstat&dawalclipmask))
                {
                    clipyou = 1;
#ifdef YAX_ENABLE
                    int16_t cb = yax_getbunch(dasect, YAX_CEILING);

                    if (cb >= 0 && (sec->ceilingstat & yax_waltosecmask(dawalclipmask)) == 0)
                    {
                        int32_t ynw = yax_getnextwall(j, YAX_CEILING);

                        if (ynw >= 0 && wall[ynw].nextsector >= 0 && (wall[ynw].cstat & dawalclipmask) == 0)
                        {
                            clipmove_tweak_pos(pos, gx, gy, x1, y1, x2, y2, &dax, &day);
                            clipyou = check_floor_curb(dasect, wall[ynw].nextsector, flordist, pos->z, dax, day);
                        }
                    }
#endif
                }
                else if (editstatus == 0)
                {
                    clipmove_tweak_pos(pos, gx, gy, x1, y1, x2, y2, &dax, &day);
                    clipyou = check_floor_curb(dasect, wal->nextsector, flordist, pos->z, dax, day);

                    if (clipyou == 0)
                    {
                        const usectortype *sec2 = (usectortype *)&sector[wal->nextsector];
                        int32_t daz2 = getceilzofslope(wal->nextsector, dax, day);

                        clipyou = ((sec2->ceilingstat&1) == 0 &&
                            pos->z <= daz2+(ceildist-1) &&
                            daz2 > getceilzofslope(dasect, dax, day)+(1<<8));
                    }
                }

            if (clipyou)
            {
                int16_t objtype;
                int32_t bsz;

                if (!curspr)
                    objtype = (int16_t) j+32768;
                else
                    objtype = (int16_t) (curspr-(uspritetype *)sprite)+49152;

                //Add 2 boxes at endpoints
                bsz = walldist; if (gx < 0) bsz = -bsz;
                addclipline(x1-bsz, y1-bsz, x1-bsz, y1+bsz, objtype);
                addclipline(x2-bsz, y2-bsz, x2-bsz, y2+bsz, objtype);
                bsz = walldist; if (gy < 0) bsz = -bsz;
                addclipline(x1+bsz, y1-bsz, x1-bsz, y1-bsz, objtype);
                addclipline(x2+bsz, y2-bsz, x2-bsz, y2-bsz, objtype);

                dax = walldist; if (dy > 0) dax = -dax;
                day = walldist; if (dx < 0) day = -day;
                addclipline(x1+dax, y1+day, x2+dax, y2+day, objtype);
            }
            else if (wal->nextsector>=0)
            {
                for (i=clipsectnum-1; i>=0; i--)
                    if (wal->nextsector == clipsectorlist[i]) break;
                if (i < 0) clipsectorlist[clipsectnum++] = wal->nextsector;
            }
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
            continue;  // next sector of this index
#endif
        for (j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[j];
            const int32_t cstat = spr->cstat;

            if ((cstat&dasprclipmask) == 0)
                continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
            if (clipsprite_try(spr, xmin, ymin, xmax, ymax))
                continue;
#endif
            x1 = spr->x; y1 = spr->y;

            switch (cstat&48)
            {
            case 0:
                if (x1 >= xmin && x1 <= xmax && y1 >= ymin && y1 <= ymax)
                {
                    const int32_t daz = spr->z + spriteheightofs(j, &k, 1);

                    if (pos->z > daz-k-flordist && pos->z < daz+ceildist)
                    {
                        int32_t bsz;
                        bsz = (spr->clipdist<<2)+walldist; if (gx < 0) bsz = -bsz;
                        addclipline(x1-bsz, y1-bsz, x1-bsz, y1+bsz, (int16_t) j+49152);
                        bsz = (spr->clipdist<<2)+walldist; if (gy < 0) bsz = -bsz;
                        addclipline(x1+bsz, y1-bsz, x1-bsz, y1-bsz, (int16_t) j+49152);
                    }
                }
                break;

            case 16:
            {
                const int32_t daz = spr->z + spriteheightofs(j, &k, 1);

                if (pos->z > daz-k-flordist && pos->z < daz+ceildist)
                {
                    get_wallspr_points(spr, &x1, &x2, &y1, &y2);

                    if (clipinsideboxline(cx, cy, x1, y1, x2, y2, rad) != 0)
                    {
                        dax = mulscale14(sintable[(spr->ang+256+512)&2047], walldist);
                        day = mulscale14(sintable[(spr->ang+256)&2047], walldist);

                        if ((x1-(pos->x))*(y2-(pos->y)) >= (x2-(pos->x))*(y1-(pos->y)))   //Front
                        {
                            addclipline(x1+dax, y1+day, x2+day, y2-dax, (int16_t) j+49152);
                        }
                        else
                        {
                            if ((cstat&64) != 0) continue;
                            addclipline(x2-dax, y2-day, x1-day, y1+dax, (int16_t) j+49152);
                        }

                        //Side blocker
                        if ((x2-x1)*((pos->x)-x1) + (y2-y1)*((pos->y)-y1) < 0)
                            addclipline(x1-day, y1+dax, x1+dax, y1+day, (int16_t) j+49152);
                        else if ((x1-x2)*((pos->x)-x2) + (y1-y2)*((pos->y)-y2) < 0)
                            addclipline(x2+day, y2-dax, x2-dax, y2-day, (int16_t) j+49152);
                    }
                }
                break;
            }

            case 32:
            {
                if (pos->z > spr->z - flordist && pos->z < spr->z + ceildist)
                {
                    if ((cstat&64) != 0)
                        if ((pos->z > spr->z) == ((cstat&8)==0))
                            continue;

                    rxi[0] = x1;
                    ryi[0] = y1;
                    get_floorspr_points((uspritetype const *) spr, 0, 0, &rxi[0], &rxi[1], &rxi[2], &rxi[3],
                        &ryi[0], &ryi[1], &ryi[2], &ryi[3]);

                    dax = mulscale14(sintable[(spr->ang-256+512)&2047], walldist);
                    day = mulscale14(sintable[(spr->ang-256)&2047], walldist);

                    if ((rxi[0]-(pos->x))*(ryi[1]-(pos->y)) < (rxi[1]-(pos->x))*(ryi[0]-(pos->y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi[1], ryi[1], rxi[0], ryi[0], rad) != 0)
                            addclipline(rxi[1]-day, ryi[1]+dax, rxi[0]+dax, ryi[0]+day, (int16_t) j+49152);
                    }
                    else if ((rxi[2]-(pos->x))*(ryi[3]-(pos->y)) < (rxi[3]-(pos->x))*(ryi[2]-(pos->y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi[3], ryi[3], rxi[2], ryi[2], rad) != 0)
                            addclipline(rxi[3]+day, ryi[3]-dax, rxi[2]-dax, ryi[2]-day, (int16_t) j+49152);
                    }

                    if ((rxi[1]-(pos->x))*(ryi[2]-(pos->y)) < (rxi[2]-(pos->x))*(ryi[1]-(pos->y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi[2], ryi[2], rxi[1], ryi[1], rad) != 0)
                            addclipline(rxi[2]-dax, ryi[2]-day, rxi[1]-day, ryi[1]+dax, (int16_t) j+49152);
                    }
                    else if ((rxi[3]-(pos->x))*(ryi[0]-(pos->y)) < (rxi[0]-(pos->x))*(ryi[3]-(pos->y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi[0], ryi[0], rxi[3], ryi[3], rad) != 0)
                            addclipline(rxi[0]+dax, ryi[0]+day, rxi[3]+day, ryi[3]-dax, (int16_t) j+49152);
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
        mapinfo_set(NULL, &origmapinfo);

        clipsectnum = origclipsectnum;
        Bmemcpy(clipsectorlist, origclipsectorlist, clipsectnum*sizeof(clipsectorlist[0]));
    }
#endif

    int32_t hitwalls[4], hitwall;
    bssize_t cnt = clipmoveboxtracenum;

    do
    {
        int32_t intx=goalx, inty=goaly;

        hitwall = raytrace(pos->x, pos->y, &intx, &inty);
        if (hitwall >= 0)
        {
            const int32_t lx = clipit[hitwall].x2-clipit[hitwall].x1;
            const int32_t ly = clipit[hitwall].y2-clipit[hitwall].y1;
            const uint64_t tempull = (int64_t) lx*(int64_t) lx + (int64_t) ly*(int64_t) ly;

            if (tempull > 0 && tempull < INT32_MAX)
            {
                tempint2 = (int32_t) tempull;

                tempint1 = (goalx-intx)*lx + (goaly-inty)*ly;

                if ((klabs(tempint1)>>11) < tempint2)
                    i = divscale20(tempint1, tempint2);
                else
                    i = 0;
                goalx = mulscale20(lx, i)+intx;
                goaly = mulscale20(ly, i)+inty;
            }

            tempint1 = dmulscale6(lx, oxvect, ly, oyvect);
            for (i=cnt+1; i<=clipmoveboxtracenum; i++)
            {
                j = hitwalls[i];
                tempint2 = dmulscale6(clipit[j].x2-clipit[j].x1, oxvect,
                    clipit[j].y2-clipit[j].y1, oyvect);
                if ((tempint1^tempint2) < 0)
                {
                    updatesector(pos->x, pos->y, sectnum);
                    return retval;
                }
            }

            keepaway(&goalx, &goaly, hitwall);
            xvect = (goalx-intx)<<14;
            yvect = (goaly-inty)<<14;

            if (cnt == clipmoveboxtracenum)
                retval = (uint16_t) clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }
        cnt--;

        pos->x = intx;
        pos->y = inty;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    for (j=0; j<clipsectnum; j++)
        if (inside(pos->x, pos->y, clipsectorlist[j]) == 1)
        {
            *sectnum = clipsectorlist[j];
            return retval;
        }

    *sectnum = -1; tempint1 = INT32_MAX;
    for (j=numsectors-1; j>=0; j--)
        if (inside(pos->x, pos->y, j) == 1)
        {
            if (sector[j].ceilingstat&2)
                tempint2 = getceilzofslope(j, pos->x, pos->y) - pos->z;
            else
                tempint2 = sector[j].ceilingz - pos->z;

            if (tempint2 > 0)
            {
                if (tempint2 < tempint1)
                {
                    *sectnum = j; tempint1 = tempint2;
                }
            }
            else
            {
                if (sector[j].floorstat&2)
                    tempint2 = pos->z - getflorzofslope(j, pos->x, pos->y);
                else
                    tempint2 = pos->z - sector[j].floorz;

                if (tempint2 <= 0)
                {
                    *sectnum = j;
                    return retval;
                }
                if (tempint2 < tempint1)
                {
                    *sectnum = j; tempint1 = tempint2;
                }
            }
        }

    return retval;
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
                        if (j < 0) clipsectorlist[clipsectnum++] = wal->nextsector;
                    }
                }

            clipsectcnt++;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return bad;
}
