// This object is shared by the editors of *all* Build games!

#include "compat.h"
#include "keys.h"
#include "build.h"
#include "cache1d.h"
#ifdef POLYMER
# include "polymer.h"
#endif
#include "editor.h"
#include "renderlayer.h"

#include "m32script.h"
#include "m32def.h"

#include "lz4.h"
#include "xxhash.h"

// XXX: This breaks editors for games other than Duke. The OSD needs a way to specify colors in abstract instead of concatenating palswap escape sequences.
#include "common_game.h"

//////////////////// Key stuff ////////////////////

#define eitherALT   (keystatus[KEYSC_LALT] || keystatus[KEYSC_RALT])
#define eitherCTRL  (keystatus[KEYSC_LCTRL] || keystatus[KEYSC_RCTRL])
#define eitherSHIFT (keystatus[KEYSC_LSHIFT] || keystatus[KEYSC_RSHIFT])

#define PRESSED_KEYSC(Key) (keystatus[KEYSC_##Key] && !(keystatus[KEYSC_##Key]=0))

////

// All these variables need verification that all relevant editor stubs are actually implementing them correctly.

char getmessage[162], getmessageleng;
int32_t getmessagetimeoff; //, charsperline;

int32_t mousxplc, mousyplc;
int32_t mouseaction;

char *scripthist[SCRIPTHISTSIZ];
int32_t scripthistend;

int32_t g_lazy_tileselector;
int32_t fixmaponsave_sprites = 1;

int32_t showambiencesounds=2;

int32_t autosave=180;

int32_t autocorruptcheck;
int32_t corruptcheck_noalreadyrefd, corruptcheck_heinum=1;
int32_t corruptcheck_game_duke3d=1;  // TODO: at startup, make conditional on which game we are editing for?
int32_t corrupt_tryfix_alt;
int32_t corruptlevel, numcorruptthings, corruptthings[MAXCORRUPTTHINGS];

////

#ifdef YAX_ENABLE
const char *yupdownwall[2] = {"upwall","downwall"};
const char *YUPDOWNWALL[2] = {"UPWALL","DOWNWALL"};
#endif

////

void drawgradient(void)
{
    int32_t i, col = editorcolors[25];
    begindrawing();
    for (i=ydim-STATUS2DSIZ+16; i<ydim && col>0; i++,col--)
        CLEARLINES2D(i, 1, (col<<24)|(col<<16)|(col<<8)|col);
    CLEARLINES2D(i, ydim-i, 0);
    enddrawing();
}

static void message_common1(const char *tmpstr)
{
    Bstrncpyz(getmessage, tmpstr, sizeof(getmessage));

    getmessageleng = Bstrlen(getmessage);
    getmessagetimeoff = totalclock + 120*2 + getmessageleng*(120/30);
//    lastmessagetime = totalclock;
}

void message(const char *fmt, ...)
{
    char tmpstr[256];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 256, fmt, va);
    va_end(va);

    message_common1(tmpstr);

    if (!mouseaction)
        OSD_Printf("%s\n", tmpstr);
}

void silentmessage(const char *fmt, ...)
{
    char tmpstr[256];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 256, fmt, va);
    va_end(va);

    message_common1(tmpstr);
}

////////// tag labeling system //////////

typedef struct
{
    hashtable_t hashtab;
    char *label[32768];
    int32_t numlabels;
} taglab_t;

static taglab_t g_taglab;

static void tstrtoupper(char *s)
{
    int32_t i;
    for (i=0; s[i]; i++)
        s[i] = Btoupper(s[i]);
}

void taglab_init()
{
    int32_t i;

    g_taglab.numlabels = 0;
    g_taglab.hashtab.size = 16384;
    hash_init(&g_taglab.hashtab);

    for (i=0; i<32768; i++)
        DO_FREE_AND_NULL(g_taglab.label[i]);
}

int32_t taglab_load(const char *filename, int32_t flags)
{
    int32_t fil, len, i;
    char buf[BMAX_PATH], *dot, *filebuf;

    taglab_init();

    len = Bstrlen(filename);
    if (len >= BMAX_PATH-1)
        return -1;
    Bmemcpy(buf, filename, len+1);

    //
    dot = Bstrrchr(buf, '.');
    if (!dot)
        dot = &buf[len];

    if (dot-buf+8 >= BMAX_PATH)
        return -1;
    Bmemcpy(dot, ".maptags", 9);
    //

    if ((fil = kopen4load(buf,flags)) == -1)
        return -1;

    len = kfilelength(fil);

    filebuf = (char *)Xmalloc(len+1);
    if (!filebuf)
    {
        kclose(fil);
        return -1;
    }

    kread(fil, filebuf, len);
    filebuf[len] = 0;
    kclose(fil);

    // ----

    {
        int32_t tag;
        char *cp=filebuf, *bp, *ep;

        while (1)
        {
#define XTAGLAB_STRINGIFY(X) TAGLAB_STRINGIFY(X)
#define TAGLAB_STRINGIFY(X) #X
            i = sscanf(cp, "%d %" XTAGLAB_STRINGIFY(TAGLAB_MAX) "s", &tag, buf);
#undef XTAGLAB_STRINGIFY
#undef TAGLAB_STRINGIFY
            if (i != 2 || !buf[0] || tag<0 || tag>=32768)
                goto nextline;

            buf[TAGLAB_MAX-1] = 0;

            i = Bstrlen(buf);
            bp = buf; while (*bp && isspace(*bp)) bp++;
            ep = &buf[i-1]; while (ep>buf && isspace(*ep)) ep--;
            ep++;

            if (!(ep > bp))
                goto nextline;
            *ep = 0;

            taglab_add(bp, tag);
//initprintf("add tag %s:%d\n", bp, tag);
nextline:
            while (*cp && *cp!='\n')
                cp++;
            while (*cp=='\r' || *cp=='\n')
                cp++;
            if (*cp == 0)
                break;
        }
    }

    // ----
    Bfree(filebuf);

    return 0;
}

int32_t taglab_save(const char *mapname)
{
    int32_t fil, len, i;
    char buf[BMAX_PATH], *dot;
    const char *label;

    if (g_taglab.numlabels==0)
        return 1;

    Bstrncpyz(buf, mapname, BMAX_PATH);

    len = Bstrlen(buf);
    //
    dot = Bstrrchr(buf, '.');
    if (!dot)
        dot = &buf[len];

    if (dot-buf+8 >= BMAX_PATH)
        return -1;
    Bmemcpy(dot, ".maptags", 9);
    //

    if ((fil = Bopen(buf,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
    {
        initprintf("Couldn't open \"%s\" for writing: %s\n", buf, strerror(errno));
        return -1;
    }

    for (i=0; i<32768; i++)
    {
        label = taglab_getlabel(i);
        if (!label)
            continue;

        len = Bsprintf(buf, "%d %s" OURNEWL, i, label);
        if (Bwrite(fil, buf, len)!=len)
            break;
    }

    Bclose(fil);

    return (i!=32768);
}

int32_t taglab_add(const char *label, int16_t tag)
{
    const char *otaglabel;
    char buf[TAGLAB_MAX];
    int32_t olabeltag, diddel=0;

    if (tag < 0)
        return -1;

    Bstrncpyz(buf, label, sizeof(buf));
    // upcase the tag for storage and comparison
    tstrtoupper(buf);

    otaglabel = g_taglab.label[tag];
    if (otaglabel)
    {
        if (!Bstrcasecmp(otaglabel, buf))
            return 0;

//        hash_delete(&g_taglab.hashtab, g_taglab.label[tag]);

        // a label having the same tag number as 'tag' is deleted
        DO_FREE_AND_NULL(g_taglab.label[tag]);
        diddel |= 1;
    }
    else
    {
        olabeltag = hash_findcase(&g_taglab.hashtab, buf);
        if (olabeltag==tag)
            return 0;

        if (olabeltag>=0)
        {
            // the label gets assigned to a new tag number ('tag deleted')
            DO_FREE_AND_NULL(g_taglab.label[olabeltag]);
            diddel |= 2;
        }
    }

    if (!diddel)
        g_taglab.numlabels++;
    g_taglab.label[tag] = Xstrdup(buf);
//initprintf("added %s %d to hash\n", g_taglab.label[tag], tag);
    hash_add(&g_taglab.hashtab, g_taglab.label[tag], tag, 1);

    return diddel;
}

const char *taglab_getlabel(int16_t tag)
{
    if (tag < 0)  // || tag>=32768 implicitly
        return NULL;

    return g_taglab.label[tag];
}

int32_t taglab_gettag(const char *label)
{
    char buf[TAGLAB_MAX];

    Bstrncpyz(buf, label, TAGLAB_MAX);

    // need to upcase since hash_findcase doesn't work as expected:
    // getting the code is still (necessarily) case-sensitive...
    tstrtoupper(buf);

    return hash_findcase(&g_taglab.hashtab, buf);
}
////////// end tag labeling system //////////

////////// UNDO/REDO SYSTEM //////////
#if M32_UNDO
mapundo_t *mapstate = NULL;

int32_t map_revision = 1;

static int32_t try_match_with_prev(int32_t idx, int32_t numsthgs, uintptr_t crc)
{
    if (mapstate->prev && mapstate->prev->num[idx]==numsthgs && mapstate->prev->crc[idx]==crc)
    {
        // found match!
        mapstate->sws[idx] = mapstate->prev->sws[idx];
        (*(int32_t *)mapstate->sws[idx])++;  // increase refcount!

        return 1;
    }

    return 0;
}

static void create_compressed_block(int32_t idx, const void *srcdata, uint32_t size, uintptr_t crc)
{
    uint32_t j;

    // allocate
    int const compressed_size = LZ4_compressBound(size);
    mapstate->sws[idx] = (char *)Xmalloc(4 + compressed_size);

    // compress & realloc
    j = LZ4_compress_default((const char*)srcdata, mapstate->sws[idx]+4, size, compressed_size);
    mapstate->sws[idx] = (char *)Xrealloc(mapstate->sws[idx], 4 + j);

    // write refcount
    *(int32_t *)mapstate->sws[idx] = 1;

    mapstate->crc[idx] = crc;
}

static void free_self_and_successors(mapundo_t *mapst)
{
    mapundo_t *cur = mapst;

    mapst->prev = NULL;  // break the back link

    while (cur->next)
        cur = cur->next;

    while (1)
    {
        int32_t i;
        mapundo_t *const prev = cur->prev;

        for (i=0; i<3; i++)
        {
            int32_t *const refcnt = (int32_t *)cur->sws[i];

            if (refcnt)
            {
                (*refcnt)--;
                if (*refcnt == 0)
                    Bfree(refcnt);  // free the block!
            }
        }

        Bfree(cur);

        if (!prev)
            break;

        cur = prev;
    }
}

// NOTE: only _consecutive_ matching (size+crc) sector/wall/sprite blocks are
// shared!
void create_map_snapshot(void)
{
    if (mapstate == NULL)
    {
        // create initial mapstate

        map_revision = 1;

        mapstate = (mapundo_t *)Xcalloc(1, sizeof(mapundo_t));
        mapstate->revision = map_revision;
        mapstate->prev = mapstate->next = NULL;
    }
    else
    {
        if (mapstate->next)
            free_self_and_successors(mapstate->next);
        // now, have no successors

        // calloc because not everything may be set in the following:
        mapstate->next = (mapundo_t *)Xcalloc(1, sizeof(mapundo_t));
        mapstate->next->prev = mapstate;

        mapstate = mapstate->next;

        mapstate->revision = ++map_revision;
    }


    fixspritesectors();

    mapstate->num[0] = numsectors;
    mapstate->num[1] = numwalls;
    mapstate->num[2] = Numsprites;


    if (numsectors)
    {
#if !defined UINTPTR_MAX
# error Need UINTPTR_MAX define to select between 32- and 64-bit functions
#endif
#if UINTPTR_MAX == 0xffffffff
        /* 32-bit */
#define XXH__ XXH32
#else
        /* 64-bit */
#define XXH__ XXH64
#endif
        uintptr_t temphash = XXH__((uint8_t *)sector, numsectors*sizeof(sectortype), numsectors*sizeof(sectortype));

        if (!try_match_with_prev(0, numsectors, temphash))
            create_compressed_block(0, sector, numsectors*sizeof(sectortype), temphash);

        if (numwalls)
        {
            temphash = XXH__((uint8_t *)wall, numwalls*sizeof(walltype), numwalls*sizeof(walltype));

            if (!try_match_with_prev(1, numwalls, temphash))
                create_compressed_block(1, wall, numwalls*sizeof(walltype), temphash);
        }

        if (Numsprites)
        {
            temphash = XXH__((uint8_t *)sprite, MAXSPRITES*sizeof(spritetype), MAXSPRITES*sizeof(spritetype));

            if (!try_match_with_prev(2, Numsprites, temphash))
            {
                int32_t i = 0;
                spritetype *const tspri = (spritetype *)Xmalloc(Numsprites*sizeof(spritetype) + 4);
                spritetype *spri = tspri;

                for (bssize_t j=0; j<MAXSPRITES && i < Numsprites; j++)
                    if (sprite[j].statnum != MAXSTATUS)
                    {
                        Bmemcpy(spri++, &sprite[j], sizeof(spritetype));
                        i++;
                    }

                create_compressed_block(2, tspri, Numsprites*sizeof(spritetype), temphash);
                Bfree(tspri);
            }
        }
#undef XXH__
    }

    CheckMapCorruption(5, 0);
}

void map_undoredo_free(void)
{
    if (mapstate)
    {
        free_self_and_successors(mapstate);
        mapstate = NULL;
    }

    map_revision = 1;
}

int32_t map_undoredo(int32_t dir)
{
    int32_t i;

    if (mapstate == NULL) return 1;

    if (dir)
    {
        if (mapstate->next == NULL || !mapstate->next->num[0]) return 1;

        //        while (map_revision+1 != mapstate->revision && mapstate->next)
        mapstate = mapstate->next;
    }
    else
    {
        if (mapstate->prev == NULL || !mapstate->prev->num[0]) return 1;

        //        while (map_revision-1 != mapstate->revision && mapstate->prev)
        mapstate = mapstate->prev;
    }

    numsectors = mapstate->num[0];
    numwalls = mapstate->num[1];
    map_revision = mapstate->revision;

    Bmemset(show2dsector, 0, sizeof(show2dsector));

    reset_highlightsector();
    reset_highlight();

    initspritelists();

    if (mapstate->num[0])
    {
        // restore sector[]
        LZ4_decompress_fast(mapstate->sws[0]+4, (char*)sector, numsectors*sizeof(sectortype));

        if (mapstate->num[1])  // restore wall[]
            LZ4_decompress_fast(mapstate->sws[1]+4, (char*)wall, numwalls*sizeof(walltype));

        if (mapstate->num[2])  // restore sprite[]
            LZ4_decompress_fast(mapstate->sws[2]+4, (char*)sprite, (mapstate->num[2])*sizeof(spritetype));
    }

    // insert sprites
    for (i=0; i<mapstate->num[2]; i++)
    {
        if ((sprite[i].cstat & 48) == 48) sprite[i].cstat &= ~48;
        Bassert((unsigned)sprite[i].sectnum < (unsigned)numsectors
                   && (unsigned)sprite[i].statnum < MAXSTATUS);
        insertsprite(sprite[i].sectnum, sprite[i].statnum);
    }

    Bassert(Numsprites == mapstate->num[2]);

#ifdef POLYMER
    if (in3dmode() && getrendermode() == REND_POLYMER)
        polymer_loadboard();
#endif
#ifdef YAX_ENABLE
    yax_update(0);
    yax_updategrays(pos.z);
#endif
    CheckMapCorruption(4, 0);

    return 0;
}
#endif

////

//// port of a.m32's corruptchk ////
// Compile wall loop checks? 0: no, 1: partial, 2: full.
#define CCHK_LOOP_CHECKS 0
// returns value from 0 (all OK) to 5 (panic!)
#define CCHK_PANIC OSDTEXT_DARKRED "PANIC!!!^O "
//#define CCHKPREF OSDTEXT_RED "^O"
#define CCHK_CORRECTED OSDTEXT_GREEN " -> "

#define CORRUPTCHK_PRINT(errlev, what, fmt, ...) do  \
{ \
    bad = max(bad, errlev); \
    if (numcorruptthings>=MAXCORRUPTTHINGS) \
        goto too_many_errors; \
    corruptthings[numcorruptthings++] = (what); \
    if (errlev >= printfromlev) \
        OSD_Printf("#%d: " fmt "\n", numcorruptthings, ## __VA_ARGS__); \
} while (0)

#ifdef YAX_ENABLE
static int32_t walls_have_equal_endpoints(int32_t w1, int32_t w2)
{
    int32_t n1 = wall[w1].point2, n2 = wall[w2].point2;

    return (wall[w1].x==wall[w2].x && wall[w1].y==wall[w2].y &&
            wall[n1].x==wall[n2].x && wall[n1].y==wall[n2].y);
}

static void correct_yax_nextwall(int32_t wallnum, int32_t bunchnum, int32_t cf, int32_t tryfixingp)
{
    int32_t i, j, startwall, endwall;
    int32_t nummatching=0, lastwall[2] = { -1, -1 };

    for (SECTORS_OF_BUNCH(bunchnum, !cf, i))
        for (WALLS_OF_SECTOR(i, j))
        {
            //  v v v shouldn't happen, 'stupidity safety'
            if (j!=wallnum && walls_have_equal_endpoints(wallnum, j))
            {
                lastwall[nummatching++] = j;
                if (nummatching==2)
                    goto outofloop;
            }
        }
outofloop:
    if (nummatching==1)
    {
        if (!tryfixingp)
        {
            OSD_Printf("    will set wall %d's %s to %d on tryfix\n",
                       wallnum, yupdownwall[cf], lastwall[0]);
        }
        else
        {
            int32_t setreverse = 0;
            yax_setnextwall(wallnum, cf, lastwall[0]);
            if (yax_getnextwall(lastwall[0], !cf) < 0)
            {
                setreverse = 1;
                yax_setnextwall(lastwall[0], !cf, wallnum);
            }

            OSD_Printf("auto-correction: set wall %d's %s to %d%s\n",
                       wallnum, yupdownwall[cf], lastwall[0], setreverse?" and its reverse link":"");
        }
    }
    else if (!tryfixingp)
    {
        if (nummatching > 1)
        {
            OSD_Printf("    found more than one matching wall: at least %d and %d\n",
                       lastwall[0], lastwall[1]);
        }
        else if (nummatching == 0)
        {
            OSD_Printf("    found no matching walls!\n");
        }
    }
}
#endif

// in reverse orientation
static int32_t walls_are_consistent(int32_t w1, int32_t w2)
{
    return (wall[w2].x==POINT2(w1).x && wall[w2].y==POINT2(w1).y &&
            wall[w1].x==POINT2(w2).x && wall[w1].y==POINT2(w2).y);
}

static void suggest_nextsector_correction(int32_t nw, int32_t j)
{
    // wall j's nextsector is inconsistent with its nextwall... what shall we do?

    if (nw>=0 && nw<numwalls)
    {
        // maybe wall[j].nextwall's nextwall is right?
        if (wall[nw].nextwall==j && walls_are_consistent(nw, j))
            OSD_Printf("   suggest setting wall[%d].nextsector to %d\n",
                       j, sectorofwall_noquick(nw));
    }

    // alternative
    if (wall[j].nextsector>=0 && wall[j].nextsector<numsectors)
    {
        int32_t w, startwall, endwall;
        for (WALLS_OF_SECTOR(wall[j].nextsector, w))
        {
            // XXX: need clearing some others?
            if (walls_are_consistent(w, j))
            {
                OSD_Printf(" ? suggest setting wall[%d].nextwall to %d\n",
                           j, w);
                break;
            }
        }
    }

    OSD_Printf(" ?? suggest making wall %d white\n", j);
}

static void do_nextsector_correction(int32_t nw, int32_t j)
{
    if (corrupt_tryfix_alt==0)
    {
        if (nw>=0 && nw<numwalls)
            if (wall[nw].nextwall==j && walls_are_consistent(nw, j))
            {
                int32_t newns = sectorofwall_noquick(nw);
                wall[j].nextsector = newns;
                OSD_Printf(CCHK_CORRECTED "auto-correction: set wall[%d].nextsector=%d\n",
                           j, newns);
            }
    }
    else if (corrupt_tryfix_alt==1)
    {
        if (wall[j].nextsector>=0 && wall[j].nextsector<numsectors)
        {
            int32_t w, startwall, endwall;
            for (WALLS_OF_SECTOR(wall[j].nextsector, w))
                if (walls_are_consistent(w, j))
                {
                    wall[j].nextwall = w;
                    OSD_Printf(CCHK_CORRECTED "auto-correction: set wall[%d].nextwall=%d\n",
                               j, w);
                    break;
                }
        }
    }
    else if (corrupt_tryfix_alt==2)
    {
        wall[j].nextwall = wall[j].nextsector = -1;
        OSD_Printf(CCHK_CORRECTED "auto-correction: made wall %d white\n", j);
    }
}


static int32_t csc_s, csc_i;
// 1: corrupt, 0: OK
static int32_t check_spritelist_consistency()
{
    int32_t ournumsprites=0;
    static uint8_t havesprite[MAXSPRITES>>3];

    csc_s = csc_i = -1;

    if (Numsprites < 0 || Numsprites > MAXSPRITES)
        return 1;

    for (bssize_t i=0; i<MAXSPRITES; i++)
    {
        const int32_t sectnum=sprite[i].sectnum, statnum=sprite[i].statnum;

        csc_i = i;

        if ((statnum==MAXSTATUS) != (sectnum==MAXSECTORS))
            return 2;  // violation of .statnum==MAXSTATUS iff .sectnum==MAXSECTORS

        if ((unsigned)statnum > MAXSTATUS || (sectnum!=MAXSECTORS && (unsigned)sectnum > (unsigned)numsectors))
            return 3;  // oob sectnum or statnum

        if (statnum != MAXSTATUS)
            ournumsprites++;
    }

    if (ournumsprites != Numsprites)
    {
        initprintf("ournumsprites=%d, Numsprites=%d\n", ournumsprites, Numsprites);
        return 4;  // counting sprites by statnum!=MAXSTATUS inconsistent with Numsprites
    }

    // SECTOR LIST

    Bmemset(havesprite, 0, (Numsprites+7)>>3);

    for (bssize_t s=0; s<numsectors; s++)
    {
        int i;
        csc_s = s;

        for (i=headspritesect[s]; i>=0; i=nextspritesect[i])
        {
            csc_i = i;

            if (i >= MAXSPRITES)
                return 5;  // oob sprite index in list, or Numsprites inconsistent

            if (havesprite[i>>3]&(1<<(i&7)))
                return 6;  // have a cycle in the list

            havesprite[i>>3] |= (1<<(i&7));

            if (sprite[i].sectnum != s)
                return 7;  // .sectnum inconsistent with list
        }

        if (i!=-1)
            return 8;  // various code checks for -1 to break loop
    }

    csc_s = -1;
    for (bssize_t i=0; i<MAXSPRITES; i++)
    {
        csc_i = i;

        if (sprite[i].statnum!=MAXSTATUS && !(havesprite[i>>3]&(1<<(i&7))))
            return 9;  // have a sprite in the world not in sector list
    }


    // STATUS LIST -- we now clear havesprite[] bits

    for (bssize_t s=0; s<MAXSTATUS; s++)
    {
        int i;
        csc_s = s;

        for (i=headspritestat[s]; i>=0; i=nextspritestat[i])
        {
            csc_i = i;

            if (i >= MAXSPRITES)
                return 10;  // oob sprite index in list, or Numsprites inconsistent

            // have a cycle in the list, or status list inconsistent with
            // sector list (*)
            if (!(havesprite[i>>3]&(1<<(i&7))))
                return 11;

            havesprite[i>>3] &= ~(1<<(i&7));

            if (sprite[i].statnum != s)
                return 12;  // .statnum inconsistent with list
        }

        if (i!=-1)
            return 13;  // various code checks for -1 to break loop
    }

    csc_s = -1;
    for (bssize_t i=0; i<Numsprites; i++)
    {
        csc_i = i;

        // Status list contains only a proper subset of the sprites in the
        // sector list.  Reverse case is handled by (*)
        if (havesprite[i>>3]&(1<<(i&7)))
            return 14;
    }

    return 0;
}

#if CCHK_LOOP_CHECKS
// Return the least wall index of the outer loop of sector <sectnum>, or
//  -1 if there is none,
//  -2 if there is more than one.
static int32_t determine_outer_loop(int32_t sectnum)
{
    int32_t j, outerloopstart = -1;

    const int32_t startwall = sector[sectnum].wallptr;
    const int32_t endwall = startwall + sector[sectnum].wallnum - 1;

    for (j=startwall; j<=endwall; j=get_nextloopstart(j))
    {
        if (clockdir(j) == CLOCKDIR_CW)
        {
            if (outerloopstart == -1)
                outerloopstart = j;
            else if (outerloopstart >= 0)
                return -2;
        }
    }

    return outerloopstart;
}
#endif

#define TRYFIX_NONE() (tryfixing == 0ull)
#define TRYFIX_CNUM(onumct) (onumct < MAXCORRUPTTHINGS && (tryfixing & (1ull<<onumct)))

int32_t CheckMapCorruption(int32_t printfromlev, uint64_t tryfixing)
{
    int32_t i, j;
    int32_t ewall=0;  // expected wall index

    int32_t errlevel=0, bad=0;
    int32_t heinumcheckstat = 0;  // 1, 2

    uint8_t *seen_nextwalls = NULL;
    int16_t *lastnextwallsource = NULL;

    numcorruptthings = 0;

    if (numsectors>MAXSECTORS)
        CORRUPTCHK_PRINT(5, 0, CCHK_PANIC "SECTOR LIMIT EXCEEDED (MAXSECTORS=%d)!!!", MAXSECTORS);

    if (numwalls>MAXWALLS)
        CORRUPTCHK_PRINT(5, 0, CCHK_PANIC "WALL LIMIT EXCEEDED (MAXWALLS=%d)!!!", MAXWALLS);

    if (numsectors>MAXSECTORS || numwalls>MAXWALLS)
    {
        corruptlevel = bad;
        return bad;
    }

    if (numsectors==0 || numwalls==0)
    {
        if (numsectors>0)
            CORRUPTCHK_PRINT(5, 0, CCHK_PANIC " Have sectors but no walls!");
        if (numwalls>0)
            CORRUPTCHK_PRINT(5, 0, CCHK_PANIC " Have walls but no sectors!");
        return bad;
    }

    if (!corruptcheck_noalreadyrefd)
    {
        seen_nextwalls = (uint8_t *)Xcalloc((numwalls+7)>>3,1);
        lastnextwallsource = (int16_t *)Xmalloc(numwalls*sizeof(lastnextwallsource[0]));
    }

    for (i=0; i<numsectors; i++)
    {
        const int32_t w0 = sector[i].wallptr;
        const int32_t numw = sector[i].wallnum;
        const int32_t endwall = w0 + numw - 1;  // inclusive

        bad = 0;

        if (w0 < 0 || w0 > numwalls)
        {
            if (w0 < 0 || w0 >= MAXWALLS)
                CORRUPTCHK_PRINT(5, CORRUPT_SECTOR|i, "SECTOR[%d].WALLPTR=%d INVALID!!!", i, w0);
            else
                CORRUPTCHK_PRINT(5, CORRUPT_SECTOR|i, "SECTOR[%d].WALLPTR=%d out of range (numwalls=%d)", i, w0, numw);
        }

        if (w0 != ewall)
            CORRUPTCHK_PRINT(4, CORRUPT_SECTOR|i, "SECTOR[%d].WALLPTR=%d inconsistent, expected %d", i, w0, ewall);

        if (numw <= 1)
            CORRUPTCHK_PRINT(5, CORRUPT_SECTOR|i, CCHK_PANIC "SECTOR[%d].WALLNUM=%d INVALID!!!", i, numw);
        else if (numw==2)
            CORRUPTCHK_PRINT(3, CORRUPT_SECTOR|i, "SECTOR[%d].WALLNUM=2, expected at least 3", i);

        ewall += numw;

        if (endwall >= numwalls)
            CORRUPTCHK_PRINT(5, CORRUPT_SECTOR|i, "SECTOR[%d]: wallptr+wallnum=%d out of range: numwalls=%d", i, endwall, numwalls);

        // inconsistent cstat&2 and heinum checker
        if (corruptcheck_heinum)
        {
            const char *cflabel[2] = {"ceiling", "floor"};

            for (j=0; j<2; j++)
            {
                const int32_t cs = !!(SECTORFLD(i,stat, j)&2);
                const int32_t hn = !!SECTORFLD(i,heinum, j);

                if (cs != hn && heinumcheckstat <= 1)
                {
                    if (numcorruptthings < MAXCORRUPTTHINGS &&
                        (heinumcheckstat==1 || (heinumcheckstat==0 && (tryfixing & (1ull<<numcorruptthings)))))
                    {
                        setslope(i, j, 0);
                        OSD_Printf(CCHK_CORRECTED "auto-correction: reset sector %d's %s slope\n",
                                   i, cflabel[j]);
                        heinumcheckstat = 1;
                    }
                    else if (corruptcheck_heinum==2 && heinumcheckstat==0)
                    {
                        CORRUPTCHK_PRINT(1, CORRUPT_SECTOR|i,
                                         "SECTOR[%d]: inconsistent %sstat&2 and heinum", i, cflabel[j]);
                    }

                    if (heinumcheckstat != 1)
                        heinumcheckstat = 2;
                }
            }
        }

        errlevel = max(errlevel, bad);

        if (bad < 4)
        {
            for (j=w0; j<=endwall; j++)
            {
                const int32_t nw = wall[j].nextwall;
                const int32_t ns = wall[j].nextsector;

                bad = 0;

                // First, some basic wall sanity checks.

                if (wall[j].point2 < w0 || wall[j].point2 > endwall)
                {
                    if (wall[j].point2 < 0 || wall[j].point2 >= MAXWALLS)
                        CORRUPTCHK_PRINT(5, CORRUPT_WALL|j, CCHK_PANIC "WALL[%d].POINT2=%d INVALID!!!",
                                         j, TrackerCast(wall[j].point2));
                    else
                        CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].POINT2=%d out of range [%d, %d]",
                                         j, TrackerCast(wall[j].point2), w0, endwall);
                }

                if (nw >= numwalls)
                {
                    const int32_t onumct = numcorruptthings;

                    if (TRYFIX_NONE())
                    {
                        if (nw >= MAXWALLS)
                            CORRUPTCHK_PRINT(5, CORRUPT_WALL|j, "WALL[%d].NEXTWALL=%d INVALID!!!",
                                             j, nw);
                        else
                            CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTWALL=%d out of range: numwalls=%d",
                                             j, nw, numwalls);
                        OSD_Printf("    will make wall %d white on tryfix\n", j);
                    }
                    else if (TRYFIX_CNUM(onumct))  // CODEDUP MAKE_WALL_WHITE
                    {
                        wall[j].nextwall = wall[j].nextsector = -1;
                        OSD_Printf(CCHK_CORRECTED "auto-correction: made wall %d white\n", j);
                    }
                }

                if (ns >= numsectors)
                {
                    const int32_t onumct = numcorruptthings;

                    if (TRYFIX_NONE())
                    {
                        if (ns >= MAXSECTORS)
                            CORRUPTCHK_PRINT(5, CORRUPT_WALL|j, "WALL[%d].NEXTSECTOR=%d INVALID!!!",
                                             j, ns);
                        else
                            CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTSECTOR=%d out of range: numsectors=%d",
                                             j, ns, numsectors);
                        OSD_Printf("    will make wall %d white on tryfix\n", j);
                    }
                    else if (TRYFIX_CNUM(onumct))  // CODEDUP MAKE_WALL_WHITE
                    {
                        wall[j].nextwall = wall[j].nextsector = -1;
                        OSD_Printf(CCHK_CORRECTED "auto-correction: made wall %d white\n", j);
                    }
                }

                if (nw>=w0 && nw<=endwall)
                    CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTWALL is its own sector's wall", j);

                if (wall[j].x==POINT2(j).x && wall[j].y==POINT2(j).y)
                    CORRUPTCHK_PRINT(3, CORRUPT_WALL|j, "WALL[%d] has length 0", j);

#ifdef YAX_ENABLE
                // Various TROR checks.
                {
                    int32_t cf;

                    for (cf=0; cf<2; cf++)
                    {
                        const int32_t ynw = yax_getnextwall(j, cf);

                        if (ynw >= 0)
                        {
                            if (ynw >= numwalls)
                                CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL %d's %s=%d out of range: numwalls=%d",
                                                 j, YUPDOWNWALL[cf], ynw, numwalls);
                            else
                            {
                                int32_t ynextwallok = 1;

                                if (j == ynw)
                                {
                                    CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL %d's %s is itself",
                                                     j, YUPDOWNWALL[cf]);
                                    ynextwallok = 0;
                                }
                                else if (!walls_have_equal_endpoints(j, ynw))
                                {
                                    CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL %d's and its %s=%d's "
                                                     "endpoints are inconsistent", j, YUPDOWNWALL[cf], ynw);
                                    ynextwallok = 0;
                                }

                                {
                                    const int16_t bunchnum = yax_getbunch(i, cf);
                                    const int32_t onumct = numcorruptthings;

                                    if (bunchnum < 0 || bunchnum >= numyaxbunches)
                                    {
                                        if (tryfixing == 0ull)
                                        {
                                            CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL %d has %s=%d, "
                                                             "but its %s bunchnum=%d is invalid",
                                                             j, YUPDOWNWALL[cf], ynw,
                                                             cf==YAX_CEILING? "ceiling":"floor", bunchnum);
                                            OSD_Printf("    will clear wall %d's %s to -1 on tryfix\n",
                                                       j, yupdownwall[cf]);

                                        }
                                        else if (tryfixing & (1ull<<onumct))
                                        {
                                            yax_setnextwall(j, cf, -1);
                                            OSD_Printf(CCHK_CORRECTED "auto-correction: cleared wall %d's %s to -1\n",
                                                       j, yupdownwall[cf]);
                                        }
                                    }
                                    else if (!ynextwallok && onumct < MAXCORRUPTTHINGS)
                                    {
                                        if ((tryfixing & (1ull<<onumct)) || 4>=printfromlev)
                                            correct_yax_nextwall(j, bunchnum, cf, tryfixing!=0ull);
                                    }
                                }

                                if (ynextwallok)
                                {
                                    const int32_t onumct = numcorruptthings;
                                    const int32_t ynwp2 = yax_getnextwall(ynw, !cf);

                                    if (ynwp2 != j)
                                    {
                                        CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL %d's %s=%d's reverse link wrong"
                                                         " (expected %d, have %d)", j, YUPDOWNWALL[cf], ynw, j, ynwp2);
                                        if (onumct < MAXCORRUPTTHINGS)
                                        {
                                            if (tryfixing & (1ull<<onumct))
                                            {
                                                yax_setnextwall(ynw, !cf, j);
                                                OSD_Printf(CCHK_CORRECTED "auto-correction: set wall %d's %s=%d's %s to %d\n",
                                                           j, yupdownwall[cf], ynw, yupdownwall[!cf], j);
                                            }
                                            else if (4>=printfromlev)
                                            {
                                                OSD_Printf("   will set wall %d's %s=%d's %s to %d on tryfix\n",
                                                           j, yupdownwall[cf], ynw, yupdownwall[!cf], j);
                                            }
                                        }
                                    }
                                }   // brace woot!
                            }
                        }
                    }
                }
#endif
                // Check for ".nextsector is its own sector"
                if (ns == i)
                {
                    if (!bad)
                    {
                        const int32_t onumct = numcorruptthings;
                        const int32_t safetoclear = (nw==j || (wall[nw].nextwall==-1 && wall[nw].nextsector==-1));

                        CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTSECTOR is its own sector", j);
                        if (onumct < MAXCORRUPTTHINGS)
                        {
                            if (tryfixing & (1ull<<onumct))
                            {
                                if (safetoclear)
                                {
                                    wall[j].nextwall = wall[j].nextsector = -1;
                                    OSD_Printf(CCHK_CORRECTED "auto-correction: cleared wall %d's nextwall"
                                               " and nextsector\n", j);
                                }
                                else
                                    do_nextsector_correction(nw, j);
                            }
                            else if (4>=printfromlev)
                            {
                                if (safetoclear)
                                    OSD_Printf("   will clear wall %d's nextwall and nextsector on tryfix\n", j);
                                else
                                    suggest_nextsector_correction(nw, j);
                            }
                        }
                    }
                }

                // Check for ".nextwall already referenced from wall ..."
                if (!corruptcheck_noalreadyrefd && nw>=0 && nw<numwalls)
                {
                    if (seen_nextwalls[nw>>3]&(1<<(nw&7)))
                    {
                        const int32_t onumct = numcorruptthings;

                        const int16_t lnws = lastnextwallsource[nw];
                        const int16_t nwnw = wall[nw].nextwall;

                        CORRUPTCHK_PRINT(3, CORRUPT_WALL|j, "WALL[%d].NEXTWALL=%d already referenced from wall %d",
                                         j, nw, lnws);

                        if (onumct < MAXCORRUPTTHINGS && (nwnw==j || nwnw==lnws))
                        {
                            const int32_t walltoclear = nwnw==j ? lnws : j;

                            if (tryfixing & (1ull<<onumct))
                            {
                                wall[walltoclear].nextsector = wall[walltoclear].nextwall = -1;
                                OSD_Printf(CCHK_CORRECTED "auto-correction: cleared wall %d's nextwall and nextsector tags to -1\n",
                                           walltoclear);
                            }
                            else if (3 >= printfromlev)
                                OSD_Printf("    wall[%d].nextwall=%d, suggest clearing wall %d's nextwall and nextsector tags to -1\n",
                                           nw, nwnw, walltoclear);
                        }
                    }
                    else
                    {
                        seen_nextwalls[nw>>3] |= 1<<(nw&7);
                        lastnextwallsource[nw] = j;
                    }
                }

                // Various checks of .nextsector and .nextwall
                if (bad < 4)
                {
                    const int32_t onumct = numcorruptthings;

                    if ((ns^nw)<0)
                    {
                        CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTSECTOR=%d and .NEXTWALL=%d inconsistent:"
                                         " missing one next pointer", j, ns, nw);
                        if (onumct < MAXCORRUPTTHINGS)
                        {
                            if (tryfixing & (1ull<<onumct))
                                do_nextsector_correction(nw, j);
                            else if (4>=printfromlev)
                                suggest_nextsector_correction(nw, j);
                        }
                    }
                    else if (ns>=0)
                    {
                        if (nw<sector[ns].wallptr || nw>=sector[ns].wallptr+sector[ns].wallnum)
                        {
                            CORRUPTCHK_PRINT(4, CORRUPT_WALL|j, "WALL[%d].NEXTWALL=%d out of .NEXTSECTOR=%d's bounds [%d .. %d]",
                                             j, nw, ns, TrackerCast(sector[ns].wallptr), sector[ns].wallptr+sector[ns].wallnum-1);
                            if (onumct < MAXCORRUPTTHINGS)
                            {
                                if (tryfixing & (1ull<<onumct))
                                    do_nextsector_correction(nw, j);
                                else if (4 >= printfromlev)
                                    suggest_nextsector_correction(nw, j);
                            }
                        }
#if 0
                        // this one usually appears together with the "already referenced" corruption
                        else if (wall[nw].nextsector != i || wall[nw].nextwall != j)
                        {
                            CORRUPTCHK_PRINT(4, CORRUPT_WALL|nw, "WALL %d nextwall's backreferences inconsistent. Expected nw=%d, ns=%d; got nw=%d, ns=%d",
                                             nw, i, j, wall[nw].nextsector, wall[nw].nextwall);
                        }
#endif
                    }
                }

                errlevel = max(errlevel, bad);
            }
        }
#if CCHK_LOOP_CHECKS
        // Wall loop checks.
        if (bad < 4 && numw >= 4)
        {
            const int32_t outerloopstart = determine_outer_loop(i);

            if (outerloopstart == -1)
                CORRUPTCHK_PRINT(2, CORRUPT_SECTOR|i, "SECTOR %d contains no outer (clockwise) loop", i);
            else if (outerloopstart == -2)
                CORRUPTCHK_PRINT(2, CORRUPT_SECTOR|i, "SECTOR %d contains more than one outer (clockwise) loops", i);
# if CCHK_LOOP_CHECKS >= 2
            else
            {
                // Now, check for whether every wall-point of every inner loop of
                // this sector (<i>) is inside the outer one.

                for (j=w0; j<=endwall; /* will step by loops */)
                {
                    const int32_t nextloopstart = get_nextloopstart(j);

                    if (j != outerloopstart)
                    {
                        int32_t k;

                        for (k=j; k<nextloopstart; k++)
                            if (!loopinside(wall[k].x, wall[k].y, outerloopstart))
                            {
                                CORRUPTCHK_PRINT(4, CORRUPT_WALL|k, "WALL %d (of sector %d) is outside the outer loop", k, i);
                                goto end_wall_loop_checks;
                            }
                    }

                    j = nextloopstart;
                }
end_wall_loop_checks:
                ;
            }
# endif
            errlevel = max(errlevel, bad);
        }
#endif
    }

    bad = 0;
    for (i=0; i<MAXSPRITES; i++)
    {
        if (sprite[i].statnum==MAXSTATUS)
            continue;

        if (sprite[i].sectnum<0 || sprite[i].sectnum>=numsectors)
            CORRUPTCHK_PRINT(4, CORRUPT_SPRITE|i, "SPRITE[%d].SECTNUM=%d. Expect problems!", i, TrackerCast(sprite[i].sectnum));

        if (sprite[i].statnum<0 || sprite[i].statnum>MAXSTATUS)
            CORRUPTCHK_PRINT(4, CORRUPT_SPRITE|i, "SPRITE[%d].STATNUM=%d. Expect problems!", i, TrackerCast(sprite[i].statnum));

        if (sprite[i].picnum<0 || sprite[i].picnum>=MAXTILES)
        {
            sprite[i].picnum = 0;
            CORRUPTCHK_PRINT(0, CORRUPT_SPRITE|i, "SPRITE[%d].PICNUM=%d out of range, resetting to 0", i, TrackerCast(sprite[i].picnum));
        }

        if (corruptcheck_game_duke3d)
        {
            const int32_t tilenum = sprite[i].picnum;

            if (tilenum >= 1 && tilenum <= 9 && (sprite[i].cstat&48))
            {
                const int32_t onumct = numcorruptthings;

                CORRUPTCHK_PRINT(1, CORRUPT_SPRITE|i, "%s sprite %d is not face-aligned",
                                 names[tilenum], i);

                if (onumct < MAXCORRUPTTHINGS)
                {
                    if (tryfixing & (1ull<<onumct))
                    {
                        sprite[i].cstat &= ~(32+16);
                        OSD_Printf(CCHK_CORRECTED "auto-correction: cleared sprite[%d].cstat bits 16 and 32\n", i);
                    }
                    else if (1 >= printfromlev)
                        OSD_Printf("   suggest clearing sprite[%d].cstat bits 16 and 32\n", i);
                }
            }
        }

        if (klabs(sprite[i].x) > BXY_MAX || klabs(sprite[i].y) > BXY_MAX)
        {
            const int32_t onumct = numcorruptthings;

            CORRUPTCHK_PRINT(3, CORRUPT_SPRITE|i, "SPRITE %d at [%d, %d] is outside the maximal grid range [%d, %d]",
                             i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), -BXY_MAX, BXY_MAX);

            if (onumct < MAXCORRUPTTHINGS)
            {
                int32_t x=0, y=0, ok=0;
                const int32_t sect = sprite[i].sectnum;

                if ((unsigned)sect < (unsigned)numsectors)
                {
                    const int32_t firstwall = sector[sect].wallptr;

                    if ((unsigned)firstwall < (unsigned)numwalls)
                    {
                        x = wall[firstwall].x;
                        y = wall[firstwall].y;
                        ok = 1;
                    }
                }

                if (!(tryfixing & (1ull<<onumct)))
                {
                    if (ok && 3 >= printfromlev)
                        OSD_Printf("   will reposition to its sector's (%d) first"
                                   " point [%d,%d] on tryfix\n", sect, x, y);
                }
                else
                {
                    if (ok)
                    {
                        sprite[i].x = x;
                        sprite[i].y = y;
                        OSD_Printf(CCHK_CORRECTED "auto-correction: repositioned sprite %d to "
                                   "its sector's (%d) first point [%d,%d]\n", i, sect, x, y);
                    }
                }
            }
        }
    }

    i = check_spritelist_consistency();
    if (i)
        CORRUPTCHK_PRINT(5, i<0?0:(CORRUPT_SPRITE|i), CCHK_PANIC "SPRITE LISTS CORRUPTED: error code %d, s=%d, i=%d!",
                         i, csc_s, csc_i);

    if (0)
    {
too_many_errors:
        if (printfromlev<=errlevel)
            OSD_Printf("!! too many errors, stopping. !!\n");
    }

    errlevel = max(errlevel, bad);

    if (errlevel)
    {
        if (printfromlev<=errlevel)
            OSD_Printf("-- corruption level: %d\n", errlevel);
        if (tryfixing)
            OSD_Printf("--\n");
    }

    if (seen_nextwalls)
    {
        Bfree(seen_nextwalls);
        Bfree(lastnextwallsource);
    }

    corruptlevel = errlevel;

    return errlevel;
}
////


////////// STATUS BAR MENU "class" //////////

#define MENU_MAX_ENTRIES (8*3)
#define MENU_ENTRY_SIZE 25  // max. length of label (including terminating NUL)

#define MENU_Y_SPACING 8
#define MENU_BASE_Y (ydim-STATUS2DSIZ+32)

#define MENU_FG_COLOR editorcolors[11]
#define MENU_BG_COLOR editorcolors[0]
#define MENU_BG_COLOR_SEL editorcolors[1]

#ifdef LUNATIC
# define MENU_HAVE_DESCRIPTION 1
#else
# define MENU_HAVE_DESCRIPTION 0
#endif

typedef struct StatusBarMenu_ {
    const char *const menuname;
    const int32_t custom_start_index;
    int32_t numentries;

    void (*process_func)(const struct StatusBarMenu_ *m, int32_t col, int32_t row);

    intptr_t auxdata[MENU_MAX_ENTRIES];
    char *description[MENU_MAX_ENTRIES];  // strdup'd description string, NULL if non
    char name[MENU_MAX_ENTRIES][MENU_ENTRY_SIZE];
} StatusBarMenu;

#define MENU_INITIALIZER_EMPTY(MenuName, ProcessFunc) \
    { MenuName, 0, 0, ProcessFunc, {}, {}, {} }
#define MENU_INITIALIZER(MenuName, CustomStartIndex, ProcessFunc, ...) \
    { MenuName, CustomStartIndex, CustomStartIndex, ProcessFunc, {}, {}, ## __VA_ARGS__ }

#ifdef LUNATIC
static void M_Clear(StatusBarMenu *m)
{
    int32_t i;

    m->numentries = 0;
    Bmemset(m->auxdata, 0, sizeof(m->auxdata));
    Bmemset(m->name, 0, sizeof(m->name));

    for (i=0; i<MENU_MAX_ENTRIES; i++)
        DO_FREE_AND_NULL(m->description[i]);
}

static int32_t M_HaveDescription(StatusBarMenu *m)
{
    int32_t i;

    for (i=0; i<MENU_MAX_ENTRIES; i++)
        if (m->description[i] != NULL)
            return 1;

    return 0;
}
#endif

// NOTE: Does not handle description strings! (Only the Lua menu uses them.)
static void M_UnregisterFunction(StatusBarMenu *m, intptr_t auxdata)
{
    int32_t i, j;

    for (i=m->custom_start_index; i<m->numentries; i++)
        if (m->auxdata[i]==auxdata)
        {
            for (j=i; j<m->numentries-1; j++)
            {
                m->auxdata[j] = m->auxdata[j+1];
                Bmemcpy(m->name[j], m->name[j+1], MENU_ENTRY_SIZE);
            }

            m->auxdata[j] = 0;
            Bmemset(m->name[j], 0, MENU_ENTRY_SIZE);

            m->numentries--;

            break;
        }
}

static void M_RegisterFunction(StatusBarMenu *m, const char *name, intptr_t auxdata, const char *description)
{
    int32_t i;

    for (i=8; i<m->numentries; i++)
    {
        if (m->auxdata[i]==auxdata)
        {
            // same auxdata, different name
            Bstrncpyz(m->name[i], name, MENU_ENTRY_SIZE);
            return;
        }
        else if (!Bstrncmp(m->name[i], name, MENU_ENTRY_SIZE))
        {
            // same name, different auxdata
            m->auxdata[i] = auxdata;
            return;
        }
    }

    if (m->numentries == MENU_MAX_ENTRIES)
        return;  // max reached

    Bstrncpyz(m->name[m->numentries], name, MENU_ENTRY_SIZE);
    m->auxdata[m->numentries] = auxdata;

#if MENU_HAVE_DESCRIPTION
    // NOTE: description only handled here (not above).
    if (description)
        m->description[m->numentries] = Xstrdup(description);
#else
    UNREFERENCED_PARAMETER(description);
#endif

    m->numentries++;
}

static void M_DisplayInitial(const StatusBarMenu *m)
{
    int32_t x = 8, y = MENU_BASE_Y+16;
    int32_t i;

    for (i=0; i<m->numentries; i++)
    {
        if (i==8 || i==16)
        {
            x += 208;
            y = MENU_BASE_Y+16;
        }

        printext16(x,y, MENU_FG_COLOR, MENU_BG_COLOR, m->name[i], 0);
        y += MENU_Y_SPACING;
    }

    printext16(m->numentries>8 ? 216 : 8, MENU_BASE_Y, MENU_FG_COLOR, -1, m->menuname, 0);

    clearkeys();
}

static void M_EnterMainLoop(StatusBarMenu *m)
{
    char disptext[80];
    const int32_t dispwidth = MENU_ENTRY_SIZE-1;

    int32_t i, col=0, row=0;
    int32_t crowmax[3] = {-1, -1, -1};
    int32_t xpos = 8, ypos = MENU_BASE_Y+16;

    if (m->numentries == 0)
    {
        printmessage16("%s menu has no entries", m->menuname);
        return;
    }

    Bmemset(disptext, 0, sizeof(disptext));

    Bassert((unsigned)m->numentries <= MENU_MAX_ENTRIES);
    for (i=0; i<=(m->numentries-1)/8; i++)
        crowmax[i] = (m->numentries >= (i+1)*8) ? 7 : (m->numentries-1)&7;

    drawgradient();
    M_DisplayInitial(m);

    while (keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        if (handleevents())
            quitevent = 0;

        _printmessage16("Select an option, press <Esc> to exit");

        if (PRESSED_KEYSC(DOWN))
        {
            if (row < crowmax[col])
            {
                printext16(xpos, ypos+row*MENU_Y_SPACING, MENU_FG_COLOR, MENU_BG_COLOR, disptext, 0);
                row++;
            }
        }
        else if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos, ypos+row*MENU_Y_SPACING, MENU_FG_COLOR, MENU_BG_COLOR, disptext, 0);
                row--;
            }
        }
        else if (PRESSED_KEYSC(LEFT))
        {
            if (col > 0)
            {
                printext16(xpos, ypos+row*8, MENU_FG_COLOR, 0, disptext, 0);
                col--;
                xpos -= 208;
                disptext[dispwidth] = 0;
                row = min(crowmax[col], row);
            }
        }
        else if (PRESSED_KEYSC(RIGHT))
        {
            if (col < 2 && crowmax[col+1]>=0)
            {
                printext16(xpos, ypos+row*8, MENU_FG_COLOR, 0, disptext, 0);
                col++;
                xpos += 208;
                disptext[dispwidth] = 0;
                row = min(crowmax[col], row);
            }
        }

        for (i=Bsnprintf(disptext, dispwidth, "%s", m->name[col*8 + row]); i < dispwidth; i++)
            disptext[i] = ' ';

        if (PRESSED_KEYSC(ENTER))
        {
            Bassert(m->process_func != NULL);
            m->process_func(m, col, row);
            break;
        }

#if MENU_HAVE_DESCRIPTION
        if (M_HaveDescription(m))
        {
            const int32_t maxrows = 20;
            int32_t r;

            for (r=0; r<maxrows+1; r++)
                printext16(16-4, 16-4 + r*8, 0, MENU_BG_COLOR,  // 71 blanks:
                           "                                                                       ", 0);
            if (m->description[col*8 + row] != NULL)
                printext16(16, 16, MENU_FG_COLOR, MENU_BG_COLOR, m->description[col*8 + row], 0);
        }
#endif
        printext16(xpos, ypos+row*MENU_Y_SPACING, MENU_FG_COLOR, MENU_BG_COLOR_SEL, disptext, 0);
        showframe(1);
    }

    printext16(xpos, ypos+row*MENU_Y_SPACING, MENU_FG_COLOR, MENU_BG_COLOR, disptext, 0);
    showframe(1);

    keystatus[KEYSC_ESC] = 0;
}

////////// SPECIAL FUNCTIONS MENU //////////

static void FuncMenu_Process(const StatusBarMenu *m, int32_t col, int32_t row);

static StatusBarMenu g_specialFuncMenu = MENU_INITIALIZER(
    "Special functions", 8, FuncMenu_Process,

    {
        "Replace invalid tiles",
        "Delete all spr of tile #",
        "Set map sky shade",
        "Set map sky height",
        "Global Z coord shift",
        "Resize selection",
        "Global shade divide",
        "Global visibility divide"
    }
);

// "External functions":

void FuncMenu(void)
{
    M_EnterMainLoop(&g_specialFuncMenu);
}

void registerMenuFunction(const char *funcname, int32_t stateidx)
{
    if (funcname)
        M_RegisterFunction(&g_specialFuncMenu, funcname, stateidx, NULL);
    else
        M_UnregisterFunction(&g_specialFuncMenu, stateidx);
}

// The processing function...

static int32_t correct_picnum(int16_t *picnumptr)
{
    int32_t picnum = *picnumptr;

    if ((unsigned)picnum >= MAXTILES || tilesiz[picnum].x <= 0)
    {
        *picnumptr = 0;
        return 1;
    }

    return 0;
}

static void FuncMenu_Process(const StatusBarMenu *m, int32_t col, int32_t row)
{
    int32_t i, j;

    switch (col)
    {

    case 1:
    case 2:
    {
        const int32_t stateidx = (Bassert(m->auxdata[col*8 + row] < g_stateCount),
                                  m->auxdata[col*8 + row]);

        const char *statename = statesinfo[stateidx].name;
        int32_t snlen = Bstrlen(statename);
        char *tmpscript = (char *)Xmalloc(1+5+1+snlen+1);

        tmpscript[0] = ' ';  // don't save in history
        Bmemcpy(&tmpscript[1], "state", 5);
        tmpscript[1+5] = ' ';
        Bmemcpy(&tmpscript[1+5+1], statename, snlen);
        tmpscript[1+5+1+snlen] = 0;

        M32RunScript(tmpscript);
        Bfree(tmpscript);

        if (vm.flags&VMFLAG_ERROR)
            printmessage16("There were errors while executing the menu function");
        else if (lastpm16time != totalclock)
            printmessage16("Menu function executed successfully");
    }
    break;

    case 0:
    {
        switch (row)
        {

        case 0:
        {
            j = 0;

            for (i=0; i<MAXSECTORS; i++)
            {
                j += correct_picnum(&sector[i].ceilingpicnum);
                j += correct_picnum(&sector[i].floorpicnum);
            }

            for (i=0; i<MAXWALLS; i++)
            {
                j += correct_picnum(&wall[i].picnum);
                j += correct_picnum(&wall[i].overpicnum);
            }
            for (i=0; i<MAXSPRITES; i++)
                j += correct_picnum(&sprite[i].picnum);

            printmessage16("Replaced %d invalid tiles",j);
        }
        break;

        case 1:
        {
            char tempbuf[64];
            Bsprintf(tempbuf,"Delete all sprites of tile #: ");
            i = getnumber16(tempbuf,-1,MAXSPRITES-1,1);
            if (i >= 0)
            {
                int32_t k = 0;
                for (j=0; j<MAXSPRITES; j++)
                    if (sprite[j].picnum == i)
                        deletesprite(j), k++;
                printmessage16("%d sprite(s) deleted",k);
            }
            else printmessage16("Aborted");
        }
        break;

        case 2:
        {
            j=getnumber16("Set map sky shade:    ",0,128,1);

            for (i=0; i<numsectors; i++)
            {
                if (sector[i].ceilingstat&1)
                    sector[i].ceilingshade = j;
            }
            printmessage16("All parallax skies adjusted");
        }
        break;

        case 3:
        {
            j=getnumber16("Set map sky height:    ",0,16777216,1);
            if (j != 0)
            {
                for (i=0; i<numsectors; i++)
                {
                    if (sector[i].ceilingstat&1)
                        sector[i].ceilingz = j;
                }
                printmessage16("All parallax skies adjusted");
            }
            else printmessage16("Aborted");
        }
        break;

        case 4:
        {
            j=getnumber16("Z offset:    ",0,16777216,1);
            if (j!=0)
            {
                for (i=0; i<numsectors; i++)
                {
                    sector[i].ceilingz += j;
                    sector[i].floorz += j;
                }
                for (i=0; i<MAXSPRITES; i++)
                    sprite[i].z += j;
                printmessage16("Map adjusted");
            }
            else printmessage16("Aborted");
        }
        break;

        case 5:
        {
            j=getnumber16("Percentage of original:    ",100,1000,0);

            if (j!=100 && j!=0)
            {
                int32_t w, currsector, start_wall, end_wall;
                double size = (j/100.f);

                for (i = 0; i < highlightsectorcnt; i++)
                {
                    currsector = highlightsector[i];
                    sector[currsector].ceilingz = (int32_t)(sector[currsector].ceilingz*size);
                    sector[currsector].floorz = (int32_t)(sector[currsector].floorz*size);

                    // Do all the walls in the sector
                    start_wall = sector[currsector].wallptr;
                    end_wall = start_wall + sector[currsector].wallnum;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        wall[w].x = (int32_t)(wall[w].x*size);
                        wall[w].y = (int32_t)(wall[w].y*size);
                        wall[w].yrepeat = min((int32_t)(wall[w].yrepeat/size),255);
                    }

                    w = headspritesect[highlightsector[i]];
                    while (w >= 0)
                    {
                        sprite[w].x = (int32_t)(sprite[w].x*size);
                        sprite[w].y = (int32_t)(sprite[w].y*size);
                        sprite[w].z = (int32_t)(sprite[w].z*size);
                        sprite[w].xrepeat = min(max((int32_t)(sprite[w].xrepeat*size),1),255);
                        sprite[w].yrepeat = min(max((int32_t)(sprite[w].yrepeat*size),1),255);
                        w = nextspritesect[w];
                    }
                }
                printmessage16("Map scaled");
            }
            else printmessage16("Aborted");
        }
        break;

        case 6:
        {
            j=getnumber16("Shade divisor:    ",1,128,1);
            if (j > 1)
            {
                for (i=0; i<numsectors; i++)
                {
                    sector[i].ceilingshade /= j;
                    sector[i].floorshade /= j;
                }
                for (i=0; i<numwalls; i++)
                    wall[i].shade /= j;
                for (i=0; i<MAXSPRITES; i++)
                    sprite[i].shade /= j;
                printmessage16("Shades adjusted");
            }
            else printmessage16("Aborted");
        }
        break;

        case 7:
        {
            j=getnumber16("Visibility divisor:    ",1,128,0);
            if (j > 1)
            {
                for (i=0; i<numsectors; i++)
                {
                    if (sector[i].visibility < 240)
                        sector[i].visibility /= j;
                    else sector[i].visibility = 240 + (sector[i].visibility>>4)/j;
                }
                printmessage16("Visibility adjusted");
            }
            else printmessage16("Aborted");
        }
        break;

        }  // switch (row)
    }
    break;  // switch (col) / case 0

    }  // switch (col)
}

#ifdef LUNATIC
typedef const char *(*luamenufunc_t)(void);

#ifdef __cplusplus
extern "C" {
#endif
extern void LM_Register(const char *name, luamenufunc_t funcptr, const char *description);
extern void LM_Clear(void);
#ifdef __cplusplus
}
#endif

static int32_t g_numLuaFuncs = 0;
static luamenufunc_t g_LuaFuncPtrs[MENU_MAX_ENTRIES];

static void LuaFuncMenu_Process(const StatusBarMenu *m, int32_t col, int32_t row)
{
    luamenufunc_t func = g_LuaFuncPtrs[col*8 + row];
    const char *errmsg;

    Bassert(func != NULL);
    errmsg = func();

    if (errmsg == NULL)
    {
        printmessage16("Lua function executed successfully");
    }
    else
    {
        printmessage16("There were errors executing the Lua function, see OSD");
        OSD_Printf("Errors executing Lua function \"%s\": %s\n", m->name[col*8 + row], errmsg);
    }
}

static StatusBarMenu g_LuaFuncMenu = MENU_INITIALIZER_EMPTY("Lua functions", LuaFuncMenu_Process);

void LuaFuncMenu(void)
{
    M_EnterMainLoop(&g_LuaFuncMenu);
}

LUNATIC_EXTERN void LM_Register(const char *name, luamenufunc_t funcptr, const char *description)
{
    if (name == NULL || g_numLuaFuncs == MENU_MAX_ENTRIES)
        return;

    g_LuaFuncPtrs[g_numLuaFuncs] = funcptr;
    M_RegisterFunction(&g_LuaFuncMenu, name, g_numLuaFuncs, description);
    g_numLuaFuncs++;
}

LUNATIC_EXTERN void LM_Clear(void)
{
    M_Clear(&g_LuaFuncMenu);

    g_numLuaFuncs = 0;
    Bmemset(g_LuaFuncPtrs, 0, sizeof(g_LuaFuncPtrs));
}
#endif
