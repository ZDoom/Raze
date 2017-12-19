//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "premap.h"
#include "prlights.h"
#include "savegame.h"
#ifdef LUNATIC
# include "lunatic_game.h"
static int32_t g_savedOK;
const char *g_failedVarname;
#endif

static OutputFileCounter savecounter;

extern char *bitptr;

#define BITPTR_POINTER 1

// For storing pointers in files.
//  back_p==0: ptr -> "small int"
//  back_p==1: "small int" -> ptr
//
//  mode: see enum in savegame.h
void G_Util_PtrToIdx(void *ptr, int32_t const count, const void *base, int32_t const mode)
{
    intptr_t *iptr = (intptr_t *)ptr;
    intptr_t const ibase = (intptr_t)base;
    int32_t const onlynon0_p = mode&P2I_ONLYNON0_BIT;

    // TODO: convert to proper offsets/indices for (a step towards) cross-
    //       compatibility between 32- and 64-bit systems in the netplay.
    //       REMEMBER to bump BYTEVERSION then.

    // WARNING: C std doesn't say that bit pattern of NULL is necessarily 0!
    if ((mode & P2I_BACK_BIT) == 0)
    {
        for (bssize_t i = 0; i < count; i++)
            if (!onlynon0_p || iptr[i])
                iptr[i] -= ibase;
    }
    else
    {
        for (bssize_t i = 0; i < count; i++)
            if (!onlynon0_p || iptr[i])
                iptr[i] += ibase;
    }
}

void G_Util_PtrToIdx2(void *ptr, int32_t const count, size_t const stride, const void *base, int32_t const mode)
{
    uint8_t *iptr = (uint8_t *)ptr;
    intptr_t const ibase = (intptr_t)base;
    int32_t const onlynon0_p = mode&P2I_ONLYNON0_BIT;

    if ((mode & P2I_BACK_BIT) == 0)
    {
        for (bssize_t i = 0; i < count; ++i)
        {
            if (!onlynon0_p || *(intptr_t *)iptr)
                *(intptr_t *)iptr -= ibase;

            iptr += stride;
        }
    }
    else
    {
        for (bssize_t i = 0; i < count; ++i)
        {
            if (!onlynon0_p || *(intptr_t *)iptr)
                *(intptr_t *)iptr += ibase;

            iptr += stride;
        }
    }
}

// TODO: sync with TROR special interpolations? (e.g. upper floor of subway)
void G_ResetInterpolations(void)
{
    int32_t k, i;

    g_interpolationCnt = 0;

    k = headspritestat[STAT_EFFECTOR];
    while (k >= 0)
    {
        switch (sprite[k].lotag)
        {
        case SE_31_FLOOR_RISE_FALL:
            G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
            break;
        case SE_32_CEILING_RISE_FALL:
            G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case SE_17_WARP_ELEVATOR:
        case SE_25_PISTON:
            G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
            G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case SE_0_ROTATING_SECTOR:
        case SE_5:
        case SE_6_SUBWAY:
        case SE_11_SWINGING_DOOR:
        case SE_14_SUBWAY_CAR:
        case SE_15_SLIDING_DOOR:
        case SE_16_REACTOR:
        case SE_26:
        case SE_30_TWO_WAY_TRAIN:
            Sect_SetInterpolation(sprite[k].sectnum);
            break;
        }

        k = nextspritestat[k];
    }

    for (i=g_interpolationCnt-1; i>=0; i--) bakipos[i] = *curipos[i];
    for (i = g_animateCnt-1; i>=0; i--)
        G_SetInterpolation(g_animatePtr[i]);
}

savebrief_t g_lastautosave, g_lastusersave, g_freshload;
int32_t g_lastAutoSaveArbitraryID = -1;
bool g_saveRequested;
savebrief_t * g_quickload;

menusave_t * g_menusaves;
size_t g_nummenusaves;

static void ReadSaveGameHeaders_CACHE1D(CACHE1D_FIND_REC *f)
{
    savehead_t h;

    for (; f != nullptr; f = f->next)
    {
        char const * fn = f->name;
        int32_t fil = kopen4loadfrommod(fn, 0);
        if (fil == -1)
            continue;

        menusave_t & msv = g_menusaves[g_nummenusaves];

        int32_t k = sv_loadheader(fil, 0, &h);
        if (k)
        {
            // old version, signal to menu code
            if (k > 0)
                msv.isOldVer = 1;
            else
            {
                kclose(fil);
                continue;
            }
            // else h.savename is all zeros (fatal failure, like wrong header
            // magic or too short header)
        }
        else
            msv.isOldVer = 0;

        if (k >= 0 && h.savename[0] != '\0')
        {
            memcpy(msv.brief.name, h.savename, ARRAY_SIZE(msv.brief.name));
            strncpy(msv.brief.path, fn, ARRAY_SIZE(msv.brief.path));

            ++g_nummenusaves;
        }

        kclose(fil);
    }
}

static size_t countcache1dfind(CACHE1D_FIND_REC *f)
{
    size_t x = 0;
    for (; f != nullptr; f = f->next)
        ++x;
    return x;
}

void ReadSaveGameHeaders(void)
{
    static char const DefaultPath[] = "/", SavePattern[] = "*.esv";

    CACHE1D_FIND_REC *findfiles_default = klistpath(DefaultPath, SavePattern, CACHE1D_FIND_FILE);

    // potentially overallocating but programmatically simple
    size_t const numfiles = countcache1dfind(findfiles_default);
    g_menusaves = (menusave_t *)Xrealloc(g_menusaves, sizeof(menusave_t) * numfiles);

    g_nummenusaves = 0;
    ReadSaveGameHeaders_CACHE1D(findfiles_default);

    klistfree(findfiles_default);
}

int32_t G_LoadSaveHeaderNew(char const *fn, savehead_t *saveh)
{
    int32_t fil = kopen4loadfrommod(fn, 0);
    if (fil == -1)
        return -1;

    int32_t i = sv_loadheader(fil, 0, saveh);
    if (i < 0)
        goto corrupt;

    int32_t screenshotofs;
    if (kread(fil, &screenshotofs, 4) != 4)
        goto corrupt;

    walock[TILE_LOADSHOT] = 255;
    if (waloff[TILE_LOADSHOT] == 0)
        allocache(&waloff[TILE_LOADSHOT], 320*200, &walock[TILE_LOADSHOT]);
    tilesiz[TILE_LOADSHOT].x = 200;
    tilesiz[TILE_LOADSHOT].y = 320;
    if (screenshotofs)
    {
        if (kdfread((char *)waloff[TILE_LOADSHOT], 320, 200, fil) != 200)
        {
            OSD_Printf("G_LoadSaveHeaderNew(): failed reading screenshot \"%s\"\n", fn);
            goto corrupt;
        }
    }
    else
    {
        Bmemset((char *)waloff[TILE_LOADSHOT], 0, 320*200);
    }
    invalidatetile(TILE_LOADSHOT, 0, 255);

    kclose(fil);
    return 0;

corrupt:
    kclose(fil);
    return 1;
}


static void sv_postudload();

// XXX: keyboard input 'blocked' after load fail? (at least ESC?)
int32_t G_LoadPlayer(savebrief_t & sv)
{
    savehead_t h;

    int32_t fil = kopen4loadfrommod(sv.path, 0);
    if (fil == -1)
        return -1;

    ready2send = 0;

    int32_t i = sv_loadheader(fil, 0, &h);
    if ((i < 0) || h.numplayers != ud.multimode)
    {
        if (i == -4 || i == -3 || i == 1)
            P_DoQuote(QUOTE_SAVE_BAD_VERSION, g_player[myconnectindex].ps);
        else if (h.numplayers!=ud.multimode)
            P_DoQuote(QUOTE_SAVE_BAD_PLAYERS, g_player[myconnectindex].ps);

        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;

        return 1;
    }

    VM_OnEvent(EVENT_PRELOADGAME, g_player[screenpeek].ps->i, screenpeek);

    // some setup first
    ud.multimode = h.numplayers;

    if (numplayers > 1)
    {
        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();
        G_DrawBackground();
        menutext_center(100, "Loading...");
        nextpage();
    }

    Net_WaitForServer();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    // non-"m_" fields will be loaded from svgm_udnetw
    ud.m_volume_number = h.volnum;
    ud.m_level_number = h.levnum;
    ud.m_player_skill = h.skill;

    // NOTE: Bmemcpy needed for SAVEGAME_MUSIC.
    EDUKE32_STATIC_ASSERT(sizeof(boardfilename) == sizeof(h.boardfn));
    Bmemcpy(boardfilename, h.boardfn, sizeof(boardfilename));

    const int mapIdx = h.volnum*MAXLEVELS + h.levnum;

    if (boardfilename[0])
        Bstrcpy(currentboardfilename, boardfilename);
    else if (g_mapInfo[mapIdx].filename)
        Bstrcpy(currentboardfilename, g_mapInfo[mapIdx].filename);

    if (currentboardfilename[0])
    {
        E_MapArt_Setup(currentboardfilename);
        append_ext_UNSAFE(currentboardfilename, ".mhk");
        loadmaphack(currentboardfilename);
    }

    Bmemcpy(currentboardfilename, boardfilename, BMAX_PATH);

    if (i == 2)
    {
        G_NewGame_EnterLevel();
    }
    else
    {
        // read the rest...
        i = sv_loadsnapshot(fil, 0, &h);
        if (i)
        {
            // in theory, we could load into an initial dump first and trivially
            // recover if things go wrong...
            Bsprintf(tempbuf, "Loading save game file \"%s\" failed (code %d), cannot recover.", sv.path, i);
            G_GameExit(tempbuf);
        }
    }
    sv_postudload();  // ud.m_XXX = ud.XXX

    VM_OnEvent(EVENT_LOADGAME, g_player[screenpeek].ps->i, screenpeek);

    kclose(fil);

    return 0;
}

////////// TIMER SAVING/RESTORING //////////

static struct {
    int32_t totalclock, totalclocklock;  // engine
    int32_t ototalclock, lockclock;  // game
} g_timers;

static void G_SaveTimers(void)
{
    g_timers.totalclock = totalclock;
    g_timers.totalclocklock = totalclocklock;
    g_timers.ototalclock = ototalclock;
    g_timers.lockclock = lockclock;
}

static void G_RestoreTimers(void)
{
    sampletimer();

    totalclock = g_timers.totalclock;
    totalclocklock = g_timers.totalclocklock;
    ototalclock = g_timers.ototalclock;
    lockclock = g_timers.lockclock;
}

//////////

#ifdef __ANDROID__
static void G_SavePalette(void)
{
    int32_t pfil;

    if ((pfil = kopen4load("palette.dat", 0)) != -1)
    {
        int len = kfilelength(pfil);

        FILE *fil = fopen("palette.dat", "rb");

        if (!fil)
        {
            fil = fopen("palette.dat", "wb");

            if (fil)
            {
                char *buf = (char *) Xaligned_alloc(16, len);

                kread(pfil, buf, len);
                fwrite(buf, len, 1, fil);
                fclose(fil);
            }
        }
        else fclose(fil);
    }
}
#endif

int32_t G_SavePlayer(savebrief_t & sv)
{
#ifdef __ANDROID__
    G_SavePalette();
#endif

    G_SaveTimers();

    Net_WaitForServer();
    ready2send = 0;

    char temp[BMAX_PATH];

    errno = 0;
    FILE *fil;

    if (sv.isValid())
    {
        if (G_ModDirSnprintf(temp, sizeof(temp), "%s", sv.path))
        {
            OSD_Printf("G_SavePlayer: file name \"%s\" too long\n", sv.path);
            goto saveproblem;
        }
        fil = fopen(temp, "wb");
    }
    else
    {
        static char const SaveName[] = "save0000.esv";
        size_t len = G_ModDirSnprintfLite(temp, sizeof(temp), SaveName);
        if (len >= sizeof(temp)-1)
        {
            OSD_Printf("G_SavePlayer: could not form automatic save path\n");
            goto saveproblem;
        }
        char * zeros = temp + (len-8);
        fil = savecounter.opennextfile(temp, zeros);
        savecounter.count++;
        // don't copy the mod dir into sv.path
        strcpy(sv.path, temp + (len-(ARRAY_SIZE(SaveName)-1)));
    }

    if (!fil)
    {
        OSD_Printf("G_SavePlayer: failed opening \"%s\" for writing: %s\n",
                   temp, strerror(errno));
        goto saveproblem;
    }

#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_resetlights();
#endif

    VM_OnEvent(EVENT_SAVEGAME, g_player[screenpeek].ps->i, screenpeek);

    // SAVE!
    sv_saveandmakesnapshot(fil, sv.name, 0, 0, 0, 0);

    fclose(fil);

    if (!g_netServer && ud.multimode < 2)
    {
#ifdef LUNATIC
        if (!g_savedOK)
            Bstrcpy(apStrings[QUOTE_RESERVED4], "^10Failed Saving Game");
        else
#endif
            Bstrcpy(apStrings[QUOTE_RESERVED4], "Game Saved");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
    }

    ready2send = 1;
    Net_WaitForServer();

    G_RestoreTimers();
    ototalclock = totalclock;

    VM_OnEvent(EVENT_POSTSAVEGAME, g_player[screenpeek].ps->i, screenpeek);

    return 0;

saveproblem:
    ready2send = 1;
    Net_WaitForServer();

    G_RestoreTimers();
    ototalclock = totalclock;

    return -1;
}

void G_LoadPlayerMaybeMulti(savebrief_t & sv)
{
    if (g_netServer || ud.multimode > 1)
    {
        Bstrcpy(apStrings[QUOTE_RESERVED4], "Multiplayer Loading Not Yet Supported");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);

//        g_player[myconnectindex].ps->gm = MODE_GAME;
    }
    else
    {
        int32_t c = G_LoadPlayer(sv);
        if (c == 0)
            g_player[myconnectindex].ps->gm = MODE_GAME;
    }
}

void G_SavePlayerMaybeMulti(savebrief_t & sv)
{
    CONFIG_WriteSetup(2);

    if (g_netServer || ud.multimode > 1)
    {
        Bstrcpy(apStrings[QUOTE_RESERVED4], "Multiplayer Saving Not Yet Supported");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
    }
    else
    {
        G_SavePlayer(sv);
    }
}

////////// GENERIC SAVING/LOADING SYSTEM //////////

typedef struct dataspec_
{
    uint32_t flags;
    void * const ptr;
    uint32_t size;
    intptr_t cnt;
} dataspec_t;

typedef struct dataspec_gv_
{
    uint32_t flags;
    void * ptr;
    uint32_t size;
    intptr_t cnt;
} dataspec_gv_t;

#define SV_DEFAULTCOMPRTHRES 8
static uint8_t savegame_diffcompress;  // 0:none, 1:Ken's LZW in cache1d.c
static uint8_t savegame_comprthres;


#define DS_DYNAMIC 1  // dereference .ptr one more time
#define DS_STRING 2
#define DS_CMP 4
// 8
#define DS_CNT(x) ((sizeof(x))<<3)  // .cnt is pointer to...
#define DS_CNT16 16
#define DS_CNT32 32
#define DS_CNTMASK (8|DS_CNT16|DS_CNT32|64)
// 64
#define DS_LOADFN 128  // .ptr is function that is run when loading
#define DS_SAVEFN 256  // .ptr is function that is run when saving
#define DS_NOCHK 1024  // don't check for diffs (and don't write out in dump) since assumed constant throughout demo
#define DS_PROTECTFN 512
#define DS_END (0x70000000)

static int32_t ds_getcnt(const dataspec_t *sp)
{
    int rv = -1;

    switch (sp->flags & DS_CNTMASK)
    {
        case 0: rv = sp->cnt; break;
        case DS_CNT16: rv = *((int16_t *)sp->cnt); break;
        case DS_CNT32: rv = *((int32_t *)sp->cnt); break;
    }

    return rv;
}

static inline void ds_get(const dataspec_t *sp, void **ptr, int32_t *cnt)
{
    *cnt = ds_getcnt(sp);
    *ptr = (sp->flags&DS_DYNAMIC) ? *((void **)sp->ptr) : sp->ptr;
}

// write state to file and/or to dump
static uint8_t *writespecdata(const dataspec_t *spec, FILE *fil, uint8_t *dump)
{
    int32_t cnt;
    void *ptr;
    const dataspec_t *sp=spec;

    for (; sp->flags!=DS_END; sp++)
    {
        if (sp->flags&(DS_SAVEFN|DS_LOADFN))
        {
            if (sp->flags&DS_SAVEFN)
                (*(void (*)(void))sp->ptr)();
            continue;
        }

        if (!fil && (sp->flags&(DS_NOCHK|DS_CMP|DS_STRING)))
            continue;

        if (sp->flags&DS_STRING)
        {
            fwrite(sp->ptr, Bstrlen((const char *)sp->ptr), 1, fil);  // not null-terminated!
            continue;
        }

        ds_get(sp, &ptr, &cnt);
        if (cnt < 0) { OSD_Printf("wsd: cnt=%d, f=0x%x.\n",cnt,sp->flags); continue; }

        if (!ptr || !cnt)
            continue;

        if (fil)
        {
            if (((sp->flags&DS_CNTMASK)==0 && sp->size*cnt<=savegame_comprthres)
                    || (sp->flags&DS_CMP))
                fwrite(ptr, sp->size, cnt, fil);
            else
                dfwrite((void *)ptr, sp->size, cnt, fil);
        }

        if (dump && (sp->flags&(DS_NOCHK|DS_CMP))==0)
        {
            Bmemcpy(dump, ptr, sp->size*cnt);
            dump += sp->size*cnt;
        }
    }
    return dump;
}

// let havedump=dumpvar&&*dumpvar
// (fil>=0 && havedump): first restore dump from file, then restore state from dump
// (fil<0 && havedump): only restore state from dump
// (fil>=0 && !havedump): only restore state from file
static int32_t readspecdata(const dataspec_t *spec, int32_t fil, uint8_t **dumpvar)
{
    int32_t cnt, i, j;
    void *ptr;
    uint8_t *dump=dumpvar?*dumpvar:NULL, *mem;
    const dataspec_t *sp=spec;
    static char cmpstrbuf[32];

    for (; sp->flags!=DS_END; sp++)
    {
        if (fil < 0 && sp->flags&(DS_NOCHK|DS_STRING|DS_CMP))  // we're updating
            continue;

        if (sp->flags&(DS_LOADFN|DS_SAVEFN))
        {
            if (sp->flags&DS_LOADFN)
                (*(void (*)())sp->ptr)();
            continue;
        }

        if (sp->flags&(DS_STRING|DS_CMP))  // DS_STRING and DS_CMP is for static data only
        {
            if (sp->flags&(DS_STRING))
                i = Bstrlen((const char *)sp->ptr);
            else
                i = sp->size*sp->cnt;

            j=kread(fil, cmpstrbuf, i);
            if (j!=i || Bmemcmp(sp->ptr, cmpstrbuf, i))
            {
                OSD_Printf("rsd: spec=%s, idx=%d:\n", (char *)spec->ptr, (int32_t)(sp-spec));
                if (j!=i)
                    OSD_Printf("    kread returned %d, expected %d.\n", j, i);
                else
                    OSD_Printf("    sp->ptr and cmpstrbuf not identical!\n");
                return -1;
            }
            continue;
        }

        ds_get(sp, &ptr, &cnt);
        if (cnt < 0) { OSD_Printf("rsd: cnt<0... wtf?\n"); return -1; }

        if (!ptr || !cnt)
            continue;

        if (fil>=0)
        {
            mem = (dump && (sp->flags&DS_NOCHK)==0) ? dump : (uint8_t *)ptr;

            if ((sp->flags&DS_CNTMASK)==0 && sp->size*cnt<=savegame_comprthres)
            {
                i = kread(fil, mem, cnt*sp->size);
                j = cnt*sp->size;
            }
            else
            {
                i = kdfread(mem, sp->size, cnt, fil);
                j = cnt;
            }
            if (i!=j)
            {
                OSD_Printf("rsd: spec=%s, idx=%d, mem=%p\n", (char *)spec->ptr, (int32_t)(sp-spec), mem);
                OSD_Printf("     (%s): read %d, expected %d!\n",
                           ((sp->flags&DS_CNTMASK)==0 && sp->size*cnt<=savegame_comprthres)?
                           "uncompressed":"compressed", i, j);

                if (i==-1)
                    OSD_Printf("     read: %s\n", strerror(errno));
                return -1;
            }
        }

        if (dump && (sp->flags&DS_NOCHK)==0)
        {
            Bmemcpy(ptr, dump, sp->size*cnt);
            dump += sp->size*cnt;
        }
    }

    if (dumpvar)
        *dumpvar = dump;
    return 0;
}

#define UINT(bits) uint##bits##_t
#define BYTES(bits) (bits>>3)
#define VAL(bits,p) (*(UINT(bits) const *)(p))
#define WVAL(bits,p) (*(UINT(bits) *)(p))

static void docmpsd(const void *ptr, void *dump, uint32_t size, uint32_t cnt, uint8_t **diffvar)
{
    uint8_t *retdiff = *diffvar;

    // Hail to the C preprocessor, baby!
#define CPSINGLEVAL(Datbits) \
        if (VAL(Datbits, ptr) != VAL(Datbits, dump))  \
        {                                             \
            WVAL(Datbits, retdiff) = WVAL(Datbits, dump) = VAL(Datbits, ptr); \
            *diffvar = retdiff+BYTES(Datbits);        \
        }

    if (cnt==1)
        switch (size)
        {
        case 8: CPSINGLEVAL(64); return;
        case 4: CPSINGLEVAL(32); return;
        case 2: CPSINGLEVAL(16); return;
        case 1: CPSINGLEVAL(8); return;
        }

#define CPELTS(Idxbits, Datbits) do \
    {                                         \
        for (i=0; i<nelts; i++)               \
        {                                     \
            if (*p!=*op)                      \
            {                                 \
                *op = *p;      \
                WVAL(Idxbits, retdiff) = i;    \
                retdiff += BYTES(Idxbits);    \
                WVAL(Datbits, retdiff) = *p;   \
                retdiff += BYTES(Datbits);    \
            }                                 \
            p++;                              \
            op++;                             \
        }                                     \
        WVAL(Idxbits, retdiff) = -1;           \
        retdiff += BYTES(Idxbits);            \
    } while (0)

#define CPDATA(Datbits) do \
    { \
        const UINT(Datbits) *p=(UINT(Datbits) const *)ptr;    \
        UINT(Datbits) *op=(UINT(Datbits) *)dump;        \
        uint32_t i, nelts=tabledivide32_noinline(size*cnt, BYTES(Datbits));    \
        if (nelts>65536)                                \
            CPELTS(32,Datbits);                         \
        else if (nelts>256)                             \
            CPELTS(16,Datbits);                         \
        else                                            \
            CPELTS(8,Datbits);                          \
    } while (0)

    if (size==8)
        CPDATA(64);
    else if ((size&3)==0)
        CPDATA(32);
    else if ((size&1)==0)
        CPDATA(16);
    else
        CPDATA(8);

    *diffvar = retdiff;
    return;

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
}

// get the number of elements to be monitored for changes
static int32_t getnumvar(const dataspec_t *spec)
{
    int32_t n=0;
    for (; spec->flags!=DS_END; spec++)
        n += (spec->flags&(DS_STRING|DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN) ? 0 : 1);
    return n;
}

// update dump at *dumpvar with new state and write diff to *diffvar
static void cmpspecdata(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    void *ptr;
    uint8_t *dump=*dumpvar, *diff=*diffvar, *tmptr;
    const dataspec_t *sp=spec;
    int32_t cnt, eltnum=0, nbytes=(getnumvar(spec)+7)>>3, l=Bstrlen((const char *)spec->ptr);

    Bmemcpy(diff, spec->ptr, l);
    diff+=l;

    while (nbytes--)
        *(diff++) = 0;  // the bitmap of indices which elements of spec have changed go here

    for (sp++; sp->flags!=DS_END; sp++)
    {
        if ((sp->flags&(DS_NOCHK|DS_STRING|DS_CMP)))
            continue;

        if (sp->flags&(DS_LOADFN|DS_SAVEFN))
        {
            if ((sp->flags&(DS_PROTECTFN))==0)
                (*(void (*)())sp->ptr)();
            continue;
        }

        ds_get(sp, &ptr, &cnt);
        if (cnt < 0) { OSD_Printf("csd: cnt=%d, f=0x%x\n", cnt, sp->flags); continue; }

        tmptr = diff;
        docmpsd(ptr, dump, sp->size, cnt, &diff);
        if (diff != tmptr)
            (*diffvar + l)[eltnum>>3] |= 1<<(eltnum&7);
        dump += sp->size*cnt;
        eltnum++;
    }

    *diffvar = diff;
    *dumpvar = dump;
    return;
}

#define VALOFS(bits,p,ofs) (*(((UINT(bits) *)(p)) + (ofs)))

// apply diff to dump, not to state! state is restored from dump afterwards.
static int32_t applydiff(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    uint8_t *dumptr=*dumpvar, *diffptr=*diffvar;
    const dataspec_t *sp=spec;
    bssize_t cnt, eltnum=-1, nbytes=(getnumvar(spec)+7)>>3, l=Bstrlen((const char *)spec->ptr);

    if (Bmemcmp(diffptr, spec->ptr, l))  // check STRING magic (sync check)
        return 1;

    diffptr += l+nbytes;

    for (sp++; sp->flags!=DS_END; sp++)
    {
        if ((sp->flags&(DS_NOCHK|DS_STRING|DS_CMP|DS_LOADFN|DS_SAVEFN)))
            continue;

        cnt = ds_getcnt(sp);
        if (cnt < 0) return 1;

        eltnum++;
        if (((*diffvar + l)[eltnum>>3]&(1<<(eltnum&7))) == 0)
        {
            dumptr += sp->size*cnt;
            continue;
        }

// ----------
#define CPSINGLEVAL(Datbits) \
            WVAL(Datbits, dumptr) = VAL(Datbits, diffptr); \
            diffptr += BYTES(Datbits); \
            dumptr += BYTES(Datbits)

        if (cnt==1)
        {
            switch (sp->size)
            {
            case 8: CPSINGLEVAL(64); continue;
            case 4: CPSINGLEVAL(32); continue;
            case 2: CPSINGLEVAL(16); continue;
            case 1: CPSINGLEVAL(8); continue;
            }
        }

#define CPELTS(Idxbits, Datbits) do \
        {                                    \
            UINT(Idxbits) idx;               \
            goto readidx_##Idxbits##_##Datbits; \
            do                               \
            {                                \
                VALOFS(Datbits, dumptr, idx) = VAL(Datbits, diffptr); \
                diffptr += BYTES(Datbits);   \
readidx_##Idxbits##_##Datbits:               \
                idx = VAL(Idxbits, diffptr); \
                diffptr += BYTES(Idxbits);   \
            } while ((int##Idxbits##_t)idx != -1);  \
        } while (0)

#define CPDATA(Datbits) do \
        {                             \
            uint32_t nelts=tabledivide32_noinline(sp->size*cnt, BYTES(Datbits)); \
            if (nelts>65536)          \
                CPELTS(32,Datbits);   \
            else if (nelts>256)       \
                CPELTS(16,Datbits);   \
            else                      \
                CPELTS(8,Datbits);    \
        } while (0)

        if (sp->size==8)
            CPDATA(64);
        else if ((sp->size&3)==0)
            CPDATA(32);
        else if ((sp->size&1)==0)
            CPDATA(16);
        else
            CPDATA(8);
        dumptr += sp->size*cnt;
// ----------

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
    }

    *diffvar = diffptr;
    *dumpvar = dumptr;
    return 0;
}

#undef VAL
#undef VALOFS
#undef BYTES
#undef UINT

// calculate size needed for dump
static uint32_t calcsz(const dataspec_t *spec)
{
    const dataspec_t *sp=spec;
    bssize_t cnt;
    uint32_t dasiz=0;

    for (; sp->flags!=DS_END; sp++)
    {
        // DS_STRINGs are used as sync checks in the diffs but not in the dump
        if ((sp->flags&(DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN|DS_STRING)))
            continue;

        cnt = ds_getcnt(sp);
        if (cnt<=0) continue;

        dasiz += cnt*sp->size;
    }

    return dasiz;
}

#ifdef USE_OPENGL
static void sv_prespriteextsave();
static void sv_postspriteext();
#endif
#if !defined LUNATIC
static void sv_calcbitptrsize();
static void sv_prescriptsave_once();
static void sv_prescriptload_once();
static void sv_postscript_once();
#else
// Recreate Lua state.
// XXX: It may matter a great deal when this is run from if the Lua code refers
// to C-side data at file scope. Such usage is strongly discouraged though.
static void sv_create_lua_state(void)
{
    El_CreateGameState();
    G_PostCreateGameState();
}
#endif
static void sv_preactordatasave();
static void sv_postactordata();
static void sv_preanimateptrsave();
static void sv_postanimateptr();
static void sv_prequote();
static void sv_quotesave();
static void sv_quoteload();
static void sv_prequoteredef();
static void sv_quoteredefsave();
static void sv_quoteredefload();
static void sv_postquoteredef();
static void sv_restsave();
static void sv_restload();
static void sv_preprojectilesave();
static void sv_postprojectilesave();
static void sv_preprojectileload();
static void sv_postprojectileload();

static projectile_t *ProjectileData;

#define SVARDATALEN \
    ((sizeof(g_player[0].user_name)+sizeof(g_player[0].pcolor)+sizeof(g_player[0].pteam) \
      +sizeof(g_player[0].frags)+sizeof(DukePlayer_t))*MAXPLAYERS)

#if !defined LUNATIC
static uint32_t savegame_bitptrsize;
#endif
static uint8_t savegame_quotedef[MAXQUOTES>>3];
static char(*savegame_quotes)[MAXQUOTELEN];
static char(*savegame_quoteredefs)[MAXQUOTELEN];
static uint8_t savegame_restdata[SVARDATALEN];

static char svgm_udnetw_string [] = "blK:udnt";
static const dataspec_t svgm_udnetw[] =
{
    { DS_STRING, (void *)svgm_udnetw_string, 0, 1 },
    { 0, &ud.multimode, sizeof(ud.multimode), 1 },
    { 0, &g_playerSpawnCnt, sizeof(g_playerSpawnCnt), 1 },
    { 0, &g_playerSpawnPoints, sizeof(g_playerSpawnPoints), 1 },

    { DS_NOCHK, &ud.volume_number, sizeof(ud.volume_number), 1 },
    { DS_NOCHK, &ud.level_number, sizeof(ud.level_number), 1 },
    { DS_NOCHK, &ud.user_map, sizeof(ud.user_map), 1 },
    { DS_NOCHK, &ud.player_skill, sizeof(ud.player_skill), 1 },
    { DS_NOCHK, &ud.music_episode, sizeof(ud.music_episode), 1 },
    { DS_NOCHK, &ud.music_level, sizeof(ud.music_level), 1 },

    { DS_NOCHK, &ud.from_bonus, sizeof(ud.from_bonus), 1 },
    { DS_NOCHK, &ud.secretlevel, sizeof(ud.secretlevel), 1 },
    { DS_NOCHK, &ud.respawn_monsters, sizeof(ud.respawn_monsters), 1 },
    { DS_NOCHK, &ud.respawn_items, sizeof(ud.respawn_items), 1 },
    { DS_NOCHK, &ud.respawn_inventory, sizeof(ud.respawn_inventory), 1 },
    { 0, &ud.god, sizeof(ud.god), 1 },
    { 0, &ud.auto_run, sizeof(ud.auto_run), 1 },
//    { DS_NOCHK, &ud.crosshair, sizeof(ud.crosshair), 1 },
    { DS_NOCHK, &ud.monsters_off, sizeof(ud.monsters_off), 1 },
    { DS_NOCHK, &ud.last_level, sizeof(ud.last_level), 1 },
    { 0, &ud.eog, sizeof(ud.eog), 1 },
    { DS_NOCHK, &ud.coop, sizeof(ud.coop), 1 },
    { DS_NOCHK, &ud.marker, sizeof(ud.marker), 1 },
    { DS_NOCHK, &ud.ffire, sizeof(ud.ffire), 1 },
    { DS_NOCHK, &ud.noexits, sizeof(ud.noexits), 1 },
    { DS_NOCHK, &ud.playerai, sizeof(ud.playerai), 1 },
    { 0, &ud.pause_on, sizeof(ud.pause_on), 1 },
    { DS_NOCHK, &currentboardfilename[0], BMAX_PATH, 1 },
//    { DS_LOADFN, (void *)&sv_postudload, 0, 1 },
    { 0, connectpoint2, sizeof(connectpoint2), 1 },
    { 0, &randomseed, sizeof(randomseed), 1 },
    { 0, &g_globalRandom, sizeof(g_globalRandom), 1 },
#ifdef LUNATIC
    // Save game tic count for Lunatic because it is exposed to userland. See
    // test/helixspawner.lua for an example.
    { 0, &g_moveThingsCount, sizeof(g_moveThingsCount), 1 },
#endif
//    { 0, &lockclock_dummy, sizeof(lockclock), 1 },
    { DS_END, 0, 0, 0 }
};

#if !defined DEBUG_MAIN_ARRAYS
# define DS_MAINAR DS_DYNAMIC
#else
# define DS_MAINAR 0
#endif

static char svgm_secwsp_string [] = "blK:swsp";
static const dataspec_t svgm_secwsp[] =
{
    { DS_STRING, (void *)svgm_secwsp_string, 0, 1 },
    { DS_NOCHK, &numwalls, sizeof(numwalls), 1 },
    { DS_MAINAR|DS_CNT(numwalls), &wall, sizeof(walltype), (intptr_t)&numwalls },
    { DS_NOCHK, &numsectors, sizeof(numsectors), 1 },
    { DS_MAINAR|DS_CNT(numsectors), &sector, sizeof(sectortype), (intptr_t)&numsectors },
    { DS_MAINAR, &sprite, sizeof(spritetype), MAXSPRITES },
#ifdef YAX_ENABLE
    { DS_NOCHK, &numyaxbunches, sizeof(numyaxbunches), 1 },
# if !defined NEW_MAP_FORMAT
    { DS_CNT(numsectors), yax_bunchnum, sizeof(yax_bunchnum[0]), (intptr_t)&numsectors },
    { DS_CNT(numwalls), yax_nextwall, sizeof(yax_nextwall[0]), (intptr_t)&numwalls },
# endif
    { DS_LOADFN|DS_PROTECTFN, (void *)&sv_postyaxload, 0, 1 },
#endif
    { 0, &Numsprites, sizeof(Numsprites), 1 },
    { 0, &tailspritefree, sizeof(tailspritefree), 1 },
    { 0, &headspritesect[0], sizeof(headspritesect[0]), MAXSECTORS+1 },
    { 0, &prevspritesect[0], sizeof(prevspritesect[0]), MAXSPRITES },
    { 0, &nextspritesect[0], sizeof(nextspritesect[0]), MAXSPRITES },
    { 0, &headspritestat[0], sizeof(headspritestat[0]), MAXSTATUS+1 },
    { 0, &prevspritestat[0], sizeof(prevspritestat[0]), MAXSPRITES },
    { 0, &nextspritestat[0], sizeof(nextspritestat[0]), MAXSPRITES },
#ifdef USE_OPENGL
    { DS_SAVEFN, (void *)&sv_prespriteextsave, 0, 1 },
#endif
    { DS_MAINAR, &spriteext, sizeof(spriteext_t), MAXSPRITES },
#ifndef NEW_MAP_FORMAT
    { DS_MAINAR, &wallext, sizeof(wallext_t), MAXWALLS },
#endif
#ifdef USE_OPENGL
    { DS_SAVEFN|DS_LOADFN, (void *)&sv_postspriteext, 0, 1 },
#endif
    { 0, &DynamicTileMap[0], sizeof(DynamicTileMap[0]), MAXTILES },  // NOCHK?
    { 0, &DynamicSoundMap[0], sizeof(DynamicSoundMap[0]), MAXSOUNDS },  // NOCHK?
    { DS_NOCHK, &g_cyclerCnt, sizeof(g_cyclerCnt), 1 },
    { DS_CNT(g_cyclerCnt), &g_cyclers[0][0], sizeof(g_cyclers[0]), (intptr_t)&g_cyclerCnt },
    { DS_NOCHK, &g_animWallCnt, sizeof(g_animWallCnt), 1 },
    { DS_CNT(g_animWallCnt), &animwall, sizeof(animwall[0]), (intptr_t)&g_animWallCnt },
    { DS_NOCHK, &g_mirrorCount, sizeof(g_mirrorCount), 1 },
    { DS_NOCHK, &g_mirrorWall[0], sizeof(g_mirrorWall[0]), ARRAY_SIZE(g_mirrorWall) },
    { DS_NOCHK, &g_mirrorSector[0], sizeof(g_mirrorSector[0]), ARRAY_SIZE(g_mirrorSector) },
// projectiles
    { 0, &SpriteProjectile[0], sizeof(projectile_t), MAXSPRITES },
    { 0, &everyothertime, sizeof(everyothertime), 1 },
    { DS_END, 0, 0, 0 }
};

static char svgm_script_string [] = "blK:scri";
static const dataspec_t svgm_script[] =
{
    { DS_STRING, (void *)svgm_script_string, 0, 1 },
#if !defined LUNATIC
    { DS_NOCHK, &g_scriptSize, sizeof(g_scriptSize), 1 },
    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)&sv_calcbitptrsize, 0, 1 },
    { DS_DYNAMIC|DS_CNT(savegame_bitptrsize)|DS_NOCHK, &bitptr, sizeof(bitptr[0]), (intptr_t)&savegame_bitptrsize },

    { DS_SAVEFN|DS_NOCHK, (void *)&sv_prescriptsave_once, 0, 1 },
#endif
    { DS_SAVEFN, (void *) &sv_preprojectilesave, 0, 1 },
    { DS_LOADFN, (void *) &sv_preprojectileload, 0, 1 },
    { DS_NOCHK, &g_tile[0], sizeof(tiledata_t), MAXTILES },
    { DS_DYNAMIC|DS_CNT(g_numProjectiles), &ProjectileData, sizeof(projectile_t), (intptr_t)&g_numProjectiles },
    { DS_SAVEFN, (void *) &sv_postprojectilesave, 0, 1 },
    { DS_LOADFN, (void *) &sv_postprojectileload, 0, 1 },
#if !defined LUNATIC
    { DS_LOADFN|DS_NOCHK, (void *)&sv_prescriptload_once, 0, 1 },
    { DS_DYNAMIC|DS_CNT(g_scriptSize)|DS_NOCHK, &apScript, sizeof(apScript[0]), (intptr_t)&g_scriptSize },
    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)&sv_postscript_once, 0, 1 },
#endif
    { DS_SAVEFN, (void *)&sv_preactordatasave, 0, 1 },
    { 0, &actor[0], sizeof(actor_t), MAXSPRITES },
    { DS_SAVEFN|DS_LOADFN, (void *)&sv_postactordata, 0, 1 },
#if defined LUNATIC
    { DS_LOADFN|DS_NOCHK, (void *)&sv_create_lua_state, 0, 1 },
#endif
    { DS_END, 0, 0, 0 }
};

static char svgm_anmisc_string [] = "blK:anms";
static char svgm_end_string [] = "savegame_end";

static const dataspec_t svgm_anmisc[] =
{
    { DS_STRING, (void *)svgm_anmisc_string, 0, 1 },
    { 0, &g_animateCnt, sizeof(g_animateCnt), 1 },
    { 0, &g_animateSect[0], sizeof(g_animateSect[0]), MAXANIMATES },
    { 0, &g_animateGoal[0], sizeof(g_animateGoal[0]), MAXANIMATES },
    { 0, &g_animateVel[0], sizeof(g_animateVel[0]), MAXANIMATES },
    { DS_SAVEFN, (void *)&sv_preanimateptrsave, 0, 1 },
    { 0, &g_animatePtr[0], sizeof(g_animatePtr[0]), MAXANIMATES },
    { DS_SAVEFN|DS_LOADFN , (void *)&sv_postanimateptr, 0, 1 },
    { 0, &g_curViewscreen, sizeof(g_curViewscreen), 1 },
    { 0, &g_origins[0], sizeof(g_origins[0]), ARRAY_SIZE(g_origins) },
    { 0, &g_spriteDeleteQueuePos, sizeof(g_spriteDeleteQueuePos), 1 },
    { DS_NOCHK, &g_deleteQueueSize, sizeof(g_deleteQueueSize), 1 },
    { DS_CNT(g_deleteQueueSize), &SpriteDeletionQueue[0], sizeof(int16_t), (intptr_t)&g_deleteQueueSize },
    { 0, &show2dsector[0], sizeof(uint8_t), MAXSECTORS>>3 },
    { DS_NOCHK, &g_cloudCnt, sizeof(g_cloudCnt), 1 },
    { 0, &g_cloudSect[0], sizeof(g_cloudSect), 1 },
    { 0, &g_cloudX, sizeof(g_cloudX), 1 },
    { 0, &g_cloudY, sizeof(g_cloudY), 1 },
    { 0, &g_pskyidx, sizeof(g_pskyidx), 1 },  // DS_NOCHK?
    { 0, &g_earthquakeTime, sizeof(g_earthquakeTime), 1 },

    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)sv_prequote, 0, 1 },
    { DS_SAVEFN, (void *)&sv_quotesave, 0, 1 },
    { DS_NOCHK, &savegame_quotedef, sizeof(savegame_quotedef), 1 },  // quotes can change during runtime, but new quote numbers cannot be allocated
    { DS_DYNAMIC, &savegame_quotes, MAXQUOTELEN, MAXQUOTES },
    { DS_LOADFN, (void *)&sv_quoteload, 0, 1 },

    { DS_NOCHK, &g_numXStrings, sizeof(g_numXStrings), 1 },
    { DS_NOCHK|DS_SAVEFN|DS_LOADFN, (void *)&sv_prequoteredef, 0, 1 },
    { DS_NOCHK|DS_SAVEFN, (void *)&sv_quoteredefsave, 0, 1 },  // quote redefinitions replace quotes at runtime, but cannot be changed after CON compilation
    { DS_NOCHK|DS_DYNAMIC|DS_CNT(g_numXStrings), &savegame_quoteredefs, MAXQUOTELEN, (intptr_t)&g_numXStrings },
    { DS_NOCHK|DS_LOADFN, (void *)&sv_quoteredefload, 0, 1 },
    { DS_NOCHK|DS_SAVEFN|DS_LOADFN, (void *)&sv_postquoteredef, 0, 1 },
#ifdef LUNATIC
    { 0, g_playerWeapon, sizeof(weapondata_t), MAXPLAYERS*MAX_WEAPONS },
#endif
    { DS_SAVEFN, (void *)&sv_restsave, 0, 1 },
    { 0, savegame_restdata, 1, sizeof(savegame_restdata) },  // sz/cnt swapped for kdfread
    { DS_LOADFN, (void *)&sv_restload, 0, 1 },

    { DS_STRING, (void *)svgm_end_string, 0, 1 },
    { DS_END, 0, 0, 0 }
};

#if !defined LUNATIC
static dataspec_gv_t *svgm_vars=NULL;
#endif
static uint8_t *dosaveplayer2(FILE *fil, uint8_t *mem);
static int32_t doloadplayer2(int32_t fil, uint8_t **memptr);
static void postloadplayer(int32_t savegamep);

// SVGM snapshot system
static uint32_t svsnapsiz;
static uint8_t *svsnapshot, *svinitsnap;
static uint32_t svdiffsiz;
static uint8_t *svdiff;

#include "gamedef.h"

#if !defined LUNATIC
# define SV_SKIPMASK (/*GAMEVAR_SYSTEM|*/GAMEVAR_READONLY|GAMEVAR_INT32PTR|    \
                      GAMEVAR_INT16PTR|GAMEVAR_UINT8PTR /*|GAMEVAR_NORESET*/ |GAMEVAR_SPECIAL)

static char svgm_vars_string [] = "blK:vars";
// setup gamevar data spec for snapshotting and diffing... gamevars must be loaded when called
static void sv_makevarspec()
{
    static char *magic = svgm_vars_string;
    int32_t i, j, numsavedvars=0, numsavedarrays=0, per;

    for (i=0; i<g_gameVarCount; i++)
        numsavedvars += (aGameVars[i].flags&SV_SKIPMASK) ? 0 : 1;

    for (i=0; i<g_gameArrayCount; i++)
        numsavedarrays += !(aGameArrays[i].flags & (GAMEARRAY_SYSTEM|GAMEARRAY_READONLY));  // SYSTEM_GAMEARRAY

    Bfree(svgm_vars);
    svgm_vars = (dataspec_gv_t *)Xmalloc((numsavedvars+numsavedarrays+2)*sizeof(dataspec_gv_t));

    svgm_vars[0].flags = DS_STRING;
    svgm_vars[0].ptr = magic;
    svgm_vars[0].cnt = 1;

    j=1;
    for (i=0; i<g_gameVarCount; i++)
    {
        if (aGameVars[i].flags&SV_SKIPMASK)
            continue;

        per = aGameVars[i].flags&GAMEVAR_USER_MASK;

        svgm_vars[j].flags = 0;
        svgm_vars[j].ptr = (per==0) ? &aGameVars[i].global : aGameVars[i].pValues;
        svgm_vars[j].size = sizeof(intptr_t);
        svgm_vars[j].cnt = (per==0) ? 1 : (per==GAMEVAR_PERPLAYER ? MAXPLAYERS : MAXSPRITES);
        j++;
    }

    for (i=0; i<g_gameArrayCount; i++)
    {
        // We must not update read-only SYSTEM_GAMEARRAY gamearrays: besides
        // being questionable by itself, sizeof(...) may be e.g. 4 whereas the
        // actual element type is int16_t (such as tilesizx[]/tilesizy[]).
        if (aGameArrays[i].flags & (GAMEARRAY_SYSTEM|GAMEARRAY_READONLY))
            continue;

        intptr_t * const plValues = aGameArrays[i].pValues;
        svgm_vars[j].flags = 0;
        svgm_vars[j].ptr = plValues;

        if (aGameArrays[i].flags & GAMEARRAY_BITMAP)
        {
            svgm_vars[j].size = (aGameArrays[i].size + 7) >> 3;
            svgm_vars[j].cnt = 1;  // assumed constant throughout demo, i.e. no RESIZEARRAY
        }
        else
        {
            svgm_vars[j].size = plValues == NULL ? 0 : sizeof(aGameArrays[0].pValues[0]);
            svgm_vars[j].cnt = aGameArrays[i].size;  // assumed constant throughout demo, i.e. no RESIZEARRAY
        }
        j++;
    }

    svgm_vars[j].flags = DS_END;
    svgm_vars[j].ptr = NULL;
    svgm_vars[j].size = svgm_vars[j].cnt = 0;
}
#endif

void sv_freemem()
{
    DO_FREE_AND_NULL(svsnapshot);
    DO_FREE_AND_NULL(svinitsnap);
    DO_FREE_AND_NULL(svdiff);
}

static void SV_AllocSnap(int32_t allocinit)
{
    sv_freemem();

    svsnapshot = (uint8_t *)Xmalloc(svsnapsiz);
    if (allocinit)
        svinitsnap = (uint8_t *)Xmalloc(svsnapsiz);
    svdiffsiz = svsnapsiz;  // theoretically it's less than could be needed in the worst case, but practically it's overkill
    svdiff = (uint8_t *)Xmalloc(svdiffsiz);
}

// make snapshot only if spot < 0 (demo)
int32_t sv_saveandmakesnapshot(FILE *fil, char const *name, int8_t spot, int8_t recdiffsp, int8_t diffcompress, int8_t synccompress)
{
    savehead_t h;

    // set a few savegame system globals
    savegame_comprthres = SV_DEFAULTCOMPRTHRES;
    savegame_diffcompress = diffcompress;

    // calculate total snapshot size
#if !defined LUNATIC
    sv_makevarspec();
    svsnapsiz = calcsz((const dataspec_t *)svgm_vars);
#else
    svsnapsiz = 0;
#endif
    svsnapsiz += calcsz(svgm_udnetw) + calcsz(svgm_secwsp) + calcsz(svgm_script) + calcsz(svgm_anmisc);


    // create header
    Bmemcpy(h.headerstr, "EDuke32SAVE", 11);
    h.majorver = SV_MAJOR_VER;
    h.minorver = SV_MINOR_VER;
    h.ptrsize = sizeof(intptr_t);
    h.bytever = BYTEVERSION;

    h.comprthres = savegame_comprthres;
    h.recdiffsp = recdiffsp;
    h.diffcompress = savegame_diffcompress;
    h.synccompress = synccompress;

    h.reccnt = 0;
    h.snapsiz = svsnapsiz;

    // the following is kinda redundant, but we save it here to be able to quickly fetch
    // it in a savegame header read
    h.numplayers = ud.multimode;
    h.volnum = ud.volume_number;
    h.levnum = ud.level_number;
    h.skill = ud.player_skill;

    const uint32_t BSZ = sizeof(h.boardfn);
    EDUKE32_STATIC_ASSERT(BSZ == sizeof(currentboardfilename));
    Bstrncpy(h.boardfn, currentboardfilename, BSZ);

    if (currentboardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
    {
        // Shoehorn currently playing music into last bytes of board name buffer.
        // SAVEGAME_MUSIC.
        if (g_musicIndex != (0*MAXLEVELS+7) && Bstrlen(h.boardfn) < BSZ-2)
        {
            h.boardfn[BSZ-2] = g_musicIndex / MAXLEVELS;
            h.boardfn[BSZ-1] = g_musicIndex % MAXLEVELS;
        }
    }

    if (spot >= 0)
    {
        // savegame
        Bstrncpyz(h.savename, name, sizeof(h.savename));
#ifdef __ANDROID__
        Bstrncpyz(h.volname, g_volumeNames[ud.volume_number], sizeof(h.volname));
        Bstrncpyz(h.skillname, g_skillNames[ud.player_skill], sizeof(h.skillname));
#endif
    }
    else
    {
        // demo

        const time_t t=time(NULL);
        struct tm *st;

        Bstrncpyz(h.savename, "EDuke32 demo", sizeof(h.savename));
        if (t>=0 && (st = localtime(&t)))
            Bsnprintf(h.savename, sizeof(h.savename), "Demo %04d%02d%02d %s",
                      st->tm_year+1900, st->tm_mon+1, st->tm_mday, s_buildRev);
    }


    // write header
    fwrite(&h, sizeof(savehead_t), 1, fil);

    // for savegames, the file offset after the screenshot goes here;
    // for demos, we keep it 0 to signify that we didn't save one
    fwrite("\0\0\0\0", 4, 1, fil);
    if (spot >= 0 && waloff[TILE_SAVESHOT])
    {
        int32_t ofs;

        // write the screenshot compressed
        dfwrite((char *)waloff[TILE_SAVESHOT], 320, 200, fil);

        // write the current file offset right after the header
        ofs = ftell(fil);
        fseek(fil, sizeof(savehead_t), SEEK_SET);
        fwrite(&ofs, 4, 1, fil);
        fseek(fil, ofs, SEEK_SET);
    }

#ifdef DEBUGGINGAIDS
    OSD_Printf("sv_saveandmakesnapshot: snapshot size: %d bytes.\n", svsnapsiz);
#endif

    if (spot >= 0)
    {
        // savegame
        dosaveplayer2(fil, NULL);
#ifdef LUNATIC
        if (!g_savedOK)
        {
            OSD_Printf("sv_saveandmakesnapshot: failed serializing Lunatic gamevar \"%s\".\n",
                       g_failedVarname);
            g_failedVarname = NULL;
            return 1;
        }
#endif
    }
    else
    {
        uint8_t *p;

        // demo
        SV_AllocSnap(0);

        p = dosaveplayer2(fil, svsnapshot);
        if (p != svsnapshot+svsnapsiz)
        {
            OSD_Printf("sv_saveandmakesnapshot: ptr-(snapshot end)=%d!\n", (int32_t)(p-(svsnapshot+svsnapsiz)));
            return 1;
        }
    }

    return 0;
}

// if file is not an EDuke32 savegame/demo, h->headerstr will be all zeros
int32_t sv_loadheader(int32_t fil, int32_t spot, savehead_t *h)
{
    int32_t havedemo = (spot < 0);

    if (kread(fil, h, sizeof(savehead_t)) != sizeof(savehead_t))
    {
        OSD_Printf("%s %d header corrupt.\n", havedemo ? "Demo":"Savegame", havedemo ? -spot : spot);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -1;
    }

    if (Bmemcmp(h->headerstr, "EDuke32SAVE", 11))
    {
        h->headerstr[sizeof(h->headerstr)-1] = 0;
        OSD_Printf("%s %d header reads \"%s\", expected \"EDuke32SAVE\".\n",
                   havedemo ? "Demo":"Savegame", havedemo ? -spot : spot, h->headerstr);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -2;
    }

    if (h->majorver != SV_MAJOR_VER || h->minorver != SV_MINOR_VER || h->bytever != BYTEVERSION)
    {
        if (havedemo)
            OSD_Printf("Incompatible demo version. Expected %d.%d.%d, found %d.%d.%d\n",
                       SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                       h->majorver, h->minorver, h->bytever);

        if (h->majorver == SV_MAJOR_VER && h->minorver == SV_MINOR_VER)
        {
            return 1;
        }
        else
        {
            Bmemset(h->headerstr, 0, sizeof(h->headerstr));
            return -3;
        }
    }

    if (h->ptrsize != sizeof(intptr_t))
    {
        if (havedemo)
            OSD_Printf("Demo incompatible. Expected pointer size %d, found %d\n",
                       (int32_t)sizeof(intptr_t), h->ptrsize);

        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -4;
    }

    return 0;
}

int32_t sv_loadsnapshot(int32_t fil, int32_t spot, savehead_t *h)
{
    uint8_t *p;
    int32_t i;

    if (spot < 0)
    {
        // demo
        i = sv_loadheader(fil, spot, h);
        if (i)
            return i;

        // Used to be in doloadplayer2(), now redundant for savegames since
        // we checked before. Multiplayer demos need still to be taken care of.
        if (h->numplayers != numplayers)
            return 9;
    }
    // else (if savegame), we just read the header and are now at offset sizeof(savehead_t)

#ifdef DEBUGGINGAIDS
    OSD_Printf("sv_loadsnapshot: snapshot size: %d bytes.\n", h->snapsiz);
#endif

    if (kread(fil, &i, 4) != 4)
    {
        OSD_Printf("sv_snapshot: couldn't read 4 bytes after header.\n");
        return 7;
    }
    if (i > 0)
    {
        if (klseek(fil, i, SEEK_SET) != i)
        {
            OSD_Printf("sv_snapshot: failed skipping over the screenshot.\n");
            return 8;
        }
    }

    savegame_comprthres = h->comprthres;

    if (spot >= 0)
    {
        // savegame
        i = doloadplayer2(fil, NULL);
        if (i)
        {
            OSD_Printf("sv_loadsnapshot: doloadplayer2() returned %d.\n", i);
            return 5;
        }
    }
    else
    {
        // demo
        savegame_diffcompress = h->diffcompress;

        svsnapsiz = h->snapsiz;

        SV_AllocSnap(1);

        p = svsnapshot;
        i = doloadplayer2(fil, &p);
        if (i)
        {
            OSD_Printf("sv_loadsnapshot: doloadplayer2() returned %d.\n", i);
            sv_freemem();
            return 5;
        }

        if (p != svsnapshot+svsnapsiz)
        {
            OSD_Printf("sv_loadsnapshot: internal error: p-(snapshot end)=%d!\n",
                       (int32_t)(p-(svsnapshot+svsnapsiz)));
            sv_freemem();
            return 6;
        }

        Bmemcpy(svinitsnap, svsnapshot, svsnapsiz);
    }

    postloadplayer((spot >= 0));

    return 0;
}


uint32_t sv_writediff(FILE *fil)
{
    uint8_t *p=svsnapshot, *d=svdiff;
    uint32_t diffsiz;

    cmpspecdata(svgm_udnetw, &p, &d);
    cmpspecdata(svgm_secwsp, &p, &d);
    cmpspecdata(svgm_script, &p, &d);
    cmpspecdata(svgm_anmisc, &p, &d);
#if !defined LUNATIC
    cmpspecdata((const dataspec_t *)svgm_vars, &p, &d);
#endif

    if (p != svsnapshot+svsnapsiz)
        OSD_Printf("sv_writediff: dump+siz=%p, p=%p!\n", svsnapshot+svsnapsiz, p);

    diffsiz = d-svdiff;

    fwrite("dIfF",4,1,fil);
    fwrite(&diffsiz, sizeof(diffsiz), 1, fil);
    if (savegame_diffcompress)
        dfwrite(svdiff, 1, diffsiz, fil);  // cnt and sz swapped
    else
        fwrite(svdiff, 1, diffsiz, fil);

    return diffsiz;
}

int32_t sv_readdiff(int32_t fil)
{
    uint8_t *p=svsnapshot, *d=svdiff, i=0; //, tbuf[4];
    int32_t diffsiz;

#if 0  // handled by the caller
    if (kread(fil, tbuf, 4)!=4)
        return -1;
    if (Bmemcmp(tbuf, "dIfF", 4))
        return 4;
#endif
    if (kread(fil, &diffsiz, sizeof(uint32_t))!=sizeof(uint32_t))
        return -1;
    if (savegame_diffcompress)
    {
        if (kdfread(svdiff, 1, diffsiz, fil) != diffsiz)  // cnt and sz swapped
            return -2;
    }
    else
    {
        if (kread(fil, svdiff, diffsiz) != diffsiz)
            return -2;
    }

    if (applydiff(svgm_udnetw, &p, &d)) return -3;
    if (applydiff(svgm_secwsp, &p, &d)) return -4;
    if (applydiff(svgm_script, &p, &d)) return -5;
    if (applydiff(svgm_anmisc, &p, &d)) return -6;
#if !defined LUNATIC
    if (applydiff((const dataspec_t *)svgm_vars, &p, &d)) return -7;
#endif

    if (p!=svsnapshot+svsnapsiz)
        i|=1;
    if (d!=svdiff+diffsiz)
        i|=2;
    if (i)
        OSD_Printf("sv_readdiff: p=%p, svsnapshot+svsnapsiz=%p; d=%p, svdiff+diffsiz=%p",
                   p, svsnapshot+svsnapsiz, d, svdiff+diffsiz);
    return i;
}

// SVGM data description
static void sv_postudload()
{
//    Bmemcpy(&boardfilename[0], &currentboardfilename[0], BMAX_PATH);  // DON'T do this in demos!
#if 1
    ud.m_level_number = ud.level_number;
    ud.m_volume_number = ud.volume_number;
    ud.m_player_skill = ud.player_skill;
    ud.m_respawn_monsters = ud.respawn_monsters;
    ud.m_respawn_items = ud.respawn_items;
    ud.m_respawn_inventory = ud.respawn_inventory;
    ud.m_monsters_off = ud.monsters_off;
    ud.m_coop = ud.coop;
    ud.m_marker = ud.marker;
    ud.m_ffire = ud.ffire;
    ud.m_noexits = ud.noexits;
#endif
}
//static int32_t lockclock_dummy;

#ifdef USE_OPENGL
static void sv_prespriteextsave()
{
    int32_t i;
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
        {
            spriteext[i].mdanimtims -= mdtims;
            if (spriteext[i].mdanimtims==0)
                spriteext[i].mdanimtims++;
        }
}
static void sv_postspriteext()
{
    int32_t i;
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
            spriteext[i].mdanimtims += mdtims;
}
#endif

#ifdef YAX_ENABLE
void sv_postyaxload(void)
{
    yax_update(numyaxbunches>0 ? 2 : 1);
}
#endif

#if !defined LUNATIC
static void sv_calcbitptrsize()
{
    savegame_bitptrsize = (g_scriptSize+7)>>3;
}
static void sv_prescriptsave_once()
{
    int32_t i;
    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
            apScript[i] = (intptr_t *)apScript[i] - apScript;

   G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);
   G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);
}
static void sv_prescriptload_once()
{
    Bfree(apScript);
    apScript = (intptr_t *)Xmalloc(g_scriptSize * sizeof(apScript[0]));
}
static void sv_postscript_once()
{
    int32_t i;

   G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);
   G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);

    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
            apScript[i] = (intptr_t)(apScript + apScript[i]);
}
#endif

static void sv_preactordatasave()
{
#ifdef POLYMER
    for (bssize_t i=0; i<MAXSPRITES; i++)
    {
        actor[i].lightptr = NULL;
        actor[i].lightId = -1;
    }
#endif
}

static void sv_postactordata()
{
#ifdef POLYMER
    for (bssize_t i=0; i<MAXSPRITES; i++)
    {
        actor[i].lightptr = NULL;
        actor[i].lightId = -1;
    }
#endif
}

static void sv_preanimateptrsave()
{
    G_Util_PtrToIdx(g_animatePtr, g_animateCnt, sector, P2I_FWD);
}
static void sv_postanimateptr()
{
    G_Util_PtrToIdx(g_animatePtr, g_animateCnt, sector, P2I_BACK);
}
static void sv_prequote()
{
    if (!savegame_quotes)
    {
        void *ptr = Xcalloc(MAXQUOTES, MAXQUOTELEN);
        savegame_quotes = (char(*)[MAXQUOTELEN])ptr;
    }
}
static void sv_quotesave()
{
    int32_t i;
    Bmemset(savegame_quotedef, 0, sizeof(savegame_quotedef));
    for (i=0; i<MAXQUOTES; i++)
        if (apStrings[i])
        {
            savegame_quotedef[i>>3] |= 1<<(i&7);
            Bmemcpy(savegame_quotes[i], apStrings[i], MAXQUOTELEN);
        }
}
static void sv_quoteload()
{
    int32_t i;
    for (i=0; i<MAXQUOTES; i++)
    {
        if (savegame_quotedef[i>>3]&(1<<(i&7)))
        {
            C_AllocQuote(i);
            Bmemcpy(apStrings[i], savegame_quotes[i], MAXQUOTELEN);
        }
    }
}

static void sv_preprojectilesave()
{
    if (ProjectileData != NULL || g_numProjectiles > 0)
        ProjectileData = (projectile_t *) Xrealloc(ProjectileData, sizeof(projectile_t) * g_numProjectiles);
#ifdef DEBUGGINGAIDS
    int onumprojectiles = g_numProjectiles;
#endif
    g_numProjectiles = 0;

    for (bssize_t i=0; i<MAXTILES; i++)
    {
        if (g_tile[i].proj)
        {
            Bmemcpy(&ProjectileData[g_numProjectiles], g_tile[i].proj, sizeof(projectile_t));
            Bmemcpy(&ProjectileData[g_numProjectiles+1], g_tile[i].defproj, sizeof(projectile_t));
            g_numProjectiles += 2;
        }
    }

#ifdef DEBUGGINGAIDS
    Bassert(g_numProjectiles == onumprojectiles);
#endif
}

static void sv_postprojectilesave()
{
//    DO_FREE_AND_NULL(ProjectileData);
}

static void sv_preprojectileload()
{
    if (ProjectileData != NULL || g_numProjectiles > 0)
        ProjectileData = (projectile_t *) Xrealloc(ProjectileData, sizeof(projectile_t) * g_numProjectiles);

    for (bssize_t i=0; i<MAXTILES; i++)
        C_FreeProjectile(i);
}

static void sv_postprojectileload()
{
#ifdef DEBUGGINGAIDS
    int onumprojectiles = g_numProjectiles;
#endif
    g_numProjectiles = 0;

    for (bssize_t i=0; i<MAXTILES; i++)
    {
        if (g_tile[i].proj)
        {
            C_AllocProjectile(i);
            Bmemcpy(g_tile[i].proj, &ProjectileData[g_numProjectiles], sizeof(projectile_t));
            Bmemcpy(g_tile[i].defproj, &ProjectileData[g_numProjectiles+1], sizeof(projectile_t));
            g_numProjectiles += 2;
        }
    }

#ifdef DEBUGGINGAIDS
    Bassert(g_numProjectiles == onumprojectiles);
#endif

//    DO_FREE_AND_NULL(ProjectileData);
}

static void sv_prequoteredef()
{
    // "+1" needed for dfwrite which doesn't handle the src==NULL && cnt==0 case
    void *ptr = Xcalloc(g_numXStrings+1, MAXQUOTELEN);
    savegame_quoteredefs = (char(*)[MAXQUOTELEN])ptr;
}
static void sv_quoteredefsave()
{
    int32_t i;
    for (i=0; i<g_numXStrings; i++)
        if (apXStrings[i])
            Bmemcpy(savegame_quoteredefs[i], apXStrings[i], MAXQUOTELEN);
}
static void sv_quoteredefload()
{
    int32_t i;
    for (i=0; i<g_numXStrings; i++)
    {
        if (!apXStrings[i])
            apXStrings[i] = (char *)Xcalloc(1,MAXQUOTELEN);
        Bmemcpy(apXStrings[i], savegame_quoteredefs[i], MAXQUOTELEN);
    }
}
static void sv_postquoteredef()
{
    Bfree(savegame_quoteredefs), savegame_quoteredefs=NULL;
}
static void sv_restsave()
{
    int32_t i;
    uint8_t *mem = savegame_restdata;
    DukePlayer_t dummy_ps;

    Bmemset(&dummy_ps, 0, sizeof(DukePlayer_t));

#define CPDAT(ptr,sz) Bmemcpy(mem, ptr, sz), mem+=sz
    for (i=0; i<MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        if (g_player[i].ps)
            CPDAT(g_player[i].ps, sizeof(DukePlayer_t));
        else
            CPDAT(&dummy_ps, sizeof(DukePlayer_t));
    }

    Bassert((savegame_restdata+SVARDATALEN)-mem == 0);
#undef CPDAT
}
static void sv_restload()
{
    int32_t i;
    uint8_t *mem = savegame_restdata;
    DukePlayer_t dummy_ps;

#define CPDAT(ptr,sz) Bmemcpy(ptr, mem, sz), mem+=sz
    for (i=0; i<MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        if (g_player[i].ps)
            CPDAT(g_player[i].ps, sizeof(DukePlayer_t));
        else
            CPDAT(&dummy_ps, sizeof(DukePlayer_t));
    }
#undef CPDAT
}

#ifdef DEBUGGINGAIDS
# define PRINTSIZE(name) do { if (mem) OSD_Printf(name ": %d\n", (int32_t)(mem-tmem)); \
        OSD_Printf(name ": %d ms\n", getticks()-t); t=getticks(); tmem=mem; } while (0)
#else
# define PRINTSIZE(name) do { } while (0)
#endif

#ifdef LUNATIC
// <levelnum>: if we're not serializing for a mapstate, -1
//  otherwise, the linearized level number
LUNATIC_CB const char *(*El_SerializeGamevars)(int32_t *slenptr, int32_t levelnum);
#endif

static uint8_t *dosaveplayer2(FILE *fil, uint8_t *mem)
{
#ifdef DEBUGGINGAIDS
    uint8_t *tmem = mem;
    int32_t t=getticks();
#endif
    mem=writespecdata(svgm_udnetw, fil, mem);  // user settings, players & net
    PRINTSIZE("ud");
    mem=writespecdata(svgm_secwsp, fil, mem);  // sector, wall, sprite
    PRINTSIZE("sws");
#ifdef LUNATIC
    {
        // Serialize Lunatic gamevars. When loading, the restoration code must
        // be present before Lua state creation in svgm_script, so save it
        // right before, too.
        int32_t slen, slen_ext;
        const char *svcode = El_SerializeGamevars(&slen, -1);

        if (slen < 0)
        {
            // Serialization failed.
            g_savedOK = 0;
            g_failedVarname = svcode;
            return mem;
        }

        fwrite("\0\1LunaGVAR\3\4", 12, 1, fil);
        slen_ext = B_LITTLE32(slen);
        fwrite(&slen_ext, sizeof(slen_ext), 1, fil);
        dfwrite(svcode, 1, slen, fil);  // cnt and sz swapped

        g_savedOK = 1;
    }
#endif
    mem=writespecdata(svgm_script, fil, mem);  // script
    PRINTSIZE("script");
    mem=writespecdata(svgm_anmisc, fil, mem);  // animates, quotes & misc.
    PRINTSIZE("animisc");

#if !defined LUNATIC
    Gv_WriteSave(fil);  // gamevars
    mem=writespecdata((const dataspec_t *)svgm_vars, 0, mem);
    PRINTSIZE("vars");
#endif

    return mem;
}

#ifdef LUNATIC
char *g_elSavecode = NULL;

static int32_t El_ReadSaveCode(int32_t fil)
{
    // Read Lua code to restore gamevar values from the savegame.
    // It will be run from Lua with its state creation later on.

    char header[12];
    int32_t slen;

    if (kread(fil, header, 12) != 12)
    {
        OSD_Printf("doloadplayer2: failed reading Lunatic gamevar header.\n");
        return -100;
    }

    if (Bmemcmp(header, "\0\1LunaGVAR\3\4", 12))
    {
        OSD_Printf("doloadplayer2: Lunatic gamevar header doesn't match.\n");
        return -101;
    }

    if (kread(fil, &slen, sizeof(slen)) != sizeof(slen))
    {
        OSD_Printf("doloadplayer2: failed reading Lunatic gamevar string size.\n");
        return -102;
    }

    slen = B_LITTLE32(slen);
    if (slen < 0)
    {
        OSD_Printf("doloadplayer2: invalid Lunatic gamevar string size %d.\n", slen);
        return -103;
    }

    if (slen > 0)
    {
        char *svcode = (char *)Xmalloc(slen+1);

        if (kdfread(svcode, 1, slen, fil) != slen)  // cnt and sz swapped
        {
            OSD_Printf("doloadplayer2: failed reading Lunatic gamevar restoration code.\n");
            Bfree(svcode);
            return -104;
        }

        svcode[slen] = 0;
        g_elSavecode = svcode;
    }

    return 0;
}

void El_FreeSaveCode(void)
{
    // Free Lunatic gamevar savegame restoration Lua code.
    Bfree(g_elSavecode);
    g_elSavecode = NULL;
}
#endif

static int32_t doloadplayer2(int32_t fil, uint8_t **memptr)
{
    uint8_t *mem = memptr ? *memptr : NULL;
#ifdef DEBUGGINGAIDS
    uint8_t *tmem=mem;
    int32_t t=getticks();
#endif
    if (readspecdata(svgm_udnetw, fil, &mem)) return -2;
    PRINTSIZE("ud");
    if (readspecdata(svgm_secwsp, fil, &mem)) return -4;
    PRINTSIZE("sws");
#ifdef LUNATIC
    {
        int32_t ret = El_ReadSaveCode(fil);
        if (ret < 0)
            return ret;
    }
#endif
    if (readspecdata(svgm_script, fil, &mem)) return -5;
    PRINTSIZE("script");
    if (readspecdata(svgm_anmisc, fil, &mem)) return -6;
    PRINTSIZE("animisc");

#if !defined LUNATIC
    int i;

    if ((i = Gv_ReadSave(fil))) return i;

    if (mem)
    {
        int32_t i;

        sv_makevarspec();
        for (i=1; svgm_vars[i].flags!=DS_END; i++)
        {
            Bmemcpy(mem, svgm_vars[i].ptr, svgm_vars[i].size*svgm_vars[i].cnt);  // careful! works because there are no DS_DYNAMIC's!
            mem += svgm_vars[i].size*svgm_vars[i].cnt;
        }
    }
    PRINTSIZE("vars");
#endif

    if (memptr)
        *memptr = mem;
    return 0;
}

int32_t sv_updatestate(int32_t frominit)
{
    uint8_t *p = svsnapshot, *pbeg=p;

    if (frominit)
        Bmemcpy(svsnapshot, svinitsnap, svsnapsiz);

    if (readspecdata(svgm_udnetw, -1, &p)) return -2;
    if (readspecdata(svgm_secwsp, -1, &p)) return -4;
    if (readspecdata(svgm_script, -1, &p)) return -5;
    if (readspecdata(svgm_anmisc, -1, &p)) return -6;

#if !defined LUNATIC
    if (readspecdata((const dataspec_t *)svgm_vars, -1, &p)) return -8;
#endif

    if (p != pbeg+svsnapsiz)
    {
        OSD_Printf("sv_updatestate: ptr-(snapshot end)=%d\n", (int32_t)(p-(pbeg+svsnapsiz)));
        return -9;
    }

    if (frominit)
        postloadplayer(0);
#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_resetlights();  // must do it after polymer_loadboard() !!!
#endif

    return 0;
}

static void postloadplayer(int32_t savegamep)
{
    int32_t i;

    //1
    if (g_player[myconnectindex].ps->over_shoulder_on != 0)
    {
        CAMERADIST = 0;
        CAMERACLOCK = 0;
        g_player[myconnectindex].ps->over_shoulder_on = 1;
    }

    //2
    screenpeek = myconnectindex;

    //2.5
    if (savegamep)
    {
        int32_t musicIdx = (ud.volume_number*MAXLEVELS) + ud.level_number;

        Bmemset(gotpic, 0, sizeof(gotpic));
        S_ClearSoundLocks();
        G_CacheMapData();

        if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
        {
            const uint32_t BSZ = sizeof(boardfilename);
            char levname[BMAX_PATH];

            G_SetupFilenameBasedMusic(levname, boardfilename, ud.level_number);

            // Potentially extract the custom music volume/level from
            // boardfilename[] stored with SAVEGAME_MUSIC.
            if (Bstrlen(boardfilename) < BSZ-2)
            {
                int32_t mi = MAXLEVELS*boardfilename[BSZ-2] + boardfilename[BSZ-1];

                if (mi != 0 && (unsigned)mi < MUS_FIRST_SPECIAL)
                    musicIdx = mi;
            }
        }

        if (ud.config.MusicToggle)
        {
            if (g_mapInfo[musicIdx].musicfn != NULL &&
                (musicIdx != g_musicIndex /* || g_mapInfo[MUS_LOADING].musicfn */))
            {
                S_StopMusic();

                g_musicIndex = musicIdx;
                S_PlayMusic(g_mapInfo[g_musicIndex].musicfn);
            }

            S_PauseMusic(0);
        }

        g_player[myconnectindex].ps->gm = MODE_GAME;
        ud.recstat = 0;

        if (g_player[myconnectindex].ps->jetpack_on)
            A_PlaySound(DUKE_JETPACK_IDLE, g_player[myconnectindex].ps->i);
    }

    //3
    P_UpdateScreenPal(g_player[myconnectindex].ps);
    g_restorePalette = -1;

    //3.5
    if (savegamep)
    {
        for (SPRITES_OF(STAT_FX, i))
            if (sprite[i].picnum == MUSICANDSFX)
            {
                T2(i) = ud.config.SoundToggle;
                T1(i) = 0;
            }

        G_UpdateScreenArea();
        FX_SetReverb(0);
    }

    //4
    if (savegamep)
    {
        if (ud.lockout)
        {
            for (i=0; i<g_animWallCnt; i++)
                switch (DYNAMICTILEMAP(wall[animwall[i].wallnum].picnum))
                {
                case FEMPIC1__STATIC:
                    wall[animwall[i].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[i].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }
#if 0
        else
        {
            for (i=0; i<g_numAnimWalls; i++)
                if (wall[animwall[i].wallnum].extra >= 0)
                    wall[animwall[i].wallnum].picnum = wall[animwall[i].wallnum].extra;
        }
#endif
    }

    //5
    G_ResetInterpolations();

    //6
    g_showShareware = 0;
    if (savegamep)
        everyothertime = 0;

    //7
    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].playerquitflag = 1;

    // ----------

    //7.5
    if (savegamep)
    {
        ready2send = 1;
        G_ClearFIFO();
        Net_WaitForServer();
    }

    //8
    // if (savegamep)  ?
#ifdef LUNATIC
    G_ResetTimers(1);
#else
    G_ResetTimers(0);
#endif

#ifdef POLYMER
    //9
    if (getrendermode() == REND_POLYMER)
        polymer_loadboard();

    // this light pointer nulling needs to be outside the getrendermode check
    // because we might be loading the savegame using another renderer but
    // change to Polymer later
    for (i=0; i<MAXSPRITES; i++)
    {
        actor[i].lightptr = NULL;
        actor[i].lightId = -1;
    }
#endif
}

////////// END GENERIC SAVING/LOADING SYSTEM //////////

