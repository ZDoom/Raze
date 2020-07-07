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
#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "premap.h"
#include "prlights.h"
#include "savegame.h"
#include "i_specialpaths.h"
#include "gamecontrol.h"
#include "version.h"
#include "raze_music.h"
#include "mapinfo.h"

#include "savegamehelp.h"
BEGIN_DUKE_NS


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


// TODO: sync with TROR special interpolations? (e.g. upper floor of subway)
void G_ResetInterpolations(void)
{
    int32_t k, i;

    numinterpolations = 0;

    k = headspritestat[STAT_EFFECTOR];
    while (k >= 0)
    {
        switch (sprite[k].lotag)
        {
        case SE_31_FLOOR_RISE_FALL:
            setinterpolation(&sector[sprite[k].sectnum].floorz);
            break;
        case SE_32_CEILING_RISE_FALL:
            setinterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case SE_17_WARP_ELEVATOR:
        case SE_25_PISTON:
            setinterpolation(&sector[sprite[k].sectnum].floorz);
            setinterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case SE_0_ROTATING_SECTOR:
        case SE_5_BOSS:
        case SE_6_SUBWAY:
        case SE_11_SWINGING_DOOR:
        case SE_14_SUBWAY_CAR:
        case SE_15_SLIDING_DOOR:
        case SE_16_REACTOR:
        case SE_26:
        case SE_30_TWO_WAY_TRAIN:
            setsectinterpolate(k);
            break;
        }

        k = nextspritestat[k];
    }

    for (i=numinterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];
    for (i = g_animateCnt-1; i>=0; i--)
        setinterpolation(g_animatePtr[i]);
}


static FileReader *OpenSavegame()
{
	auto file = ReadSavegameChunk("snapshot.dat");
	if (!file.isOpen())
	{
		FinishSavegameRead();
		return nullptr;
	}
	return new FileReader(std::move(file));
}


static void sv_postudload();


// XXX: keyboard input 'blocked' after load fail? (at least ESC?)
int32_t G_LoadPlayer(const char *path)
{
    auto fil = OpenSavegame();

    if (!fil)
        return -1;

    ready2send = 0;

    savehead_t h;
    int status = sv_loadheader(*fil, 0, &h);

    if (status < 0 || h.numplayers != ud.multimode)
    {
        if (status == -4 || status == -3 || status == 1)
            FTA(QUOTE_SAVE_BAD_VERSION, g_player[myconnectindex].ps);
        else if (h.numplayers != ud.multimode)
            FTA(QUOTE_SAVE_BAD_PLAYERS, g_player[myconnectindex].ps);

        ototalclock = totalclock;
        ready2send = 1;

		delete fil;
		FinishSavegameRead();
		return 1;
    }

    // some setup first
    ud.multimode = h.numplayers;
	S_PauseSounds(true);

    Net_WaitForEverybody();

    FX_StopAllSounds();

    ud.m_player_skill = h.skill;

    // NOTE: Bmemcpy needed for SAVEGAME_MUSIC.
    strcpy(boardfilename, currentLevel->fileName);

	char workbuffer[BMAX_PATH];
	Bstrcpy(workbuffer, currentLevel->fileName);

    if (workbuffer[0])
    {
        artSetupMapArt(workbuffer);
        append_ext_UNSAFE(workbuffer, ".mhk");
        engineLoadMHK(workbuffer);
    }

    if ((status = sv_loadsnapshot(*fil, 0, &h)))  // read the rest...
    {
        // in theory, we could load into an initial dump first and trivially
        // recover if things go wrong...
        I_Error("Loading save game file \"%s\" failed (code %d), cannot recover.", path, status);
    }
	
    sv_postudload();  // ud.m_XXX = ud.XXX

	delete fil;
	FinishSavegameRead();
	return 0;
}

////////// TIMER SAVING/RESTORING //////////

static struct {
    int32_t totalclock, totalclocklock;  // engine
    int32_t ototalclock, lockclock;  // game
} g_timers;

static void G_SaveTimers(void)
{
    g_timers.totalclock     = (int32_t) totalclock;
    g_timers.totalclocklock = (int32_t) totalclocklock;
    g_timers.ototalclock    = (int32_t) ototalclock;
    g_timers.lockclock      = (int32_t) lockclock;
}

static void G_RestoreTimers(void)
{
    totalclock     = g_timers.totalclock;
    totalclocklock = g_timers.totalclocklock;
    ototalclock    = g_timers.ototalclock;
    lockclock      = g_timers.lockclock;
}

bool G_SavePlayer(FSaveGameNode *sv)
{
    G_SaveTimers();

    Net_WaitForEverybody();
    ready2send = 0;

	FString fn;

	errno = 0;
	FileWriter *fil;

	fil = WriteSavegameChunk("snapshot.dat");
	// The above call cannot fail.
	{
		auto& fw = *fil;

        // SAVE!
        sv_saveandmakesnapshot(fw, 0, 0);


		fw.Close();
		bool res = FinishSavegameWrite();

		if (!g_netServer && ud.multimode < 2)
		{
			Printf("Saved: %s\n", fn.GetChars());
			quoteMgr.InitializeQuote(QUOTE_RESERVED4, "Game Saved");
			FTA(QUOTE_RESERVED4, g_player[myconnectindex].ps);
		}
		
		ready2send = 1;
		Net_WaitForEverybody();
		
		G_RestoreTimers();
		ototalclock = totalclock;
		
		return res;
	}
}

bool GameInterface::LoadGame(FSaveGameNode* sv)
{
    if (g_netServer || ud.multimode > 1)
    {
		quoteMgr.InitializeQuote(QUOTE_RESERVED4, "Multiplayer Loading Not Yet Supported");
        FTA(QUOTE_RESERVED4, g_player[myconnectindex].ps);

//        g_player[myconnectindex].ps->gm = MODE_GAME;
        return false;
    }
    else
    {
        int32_t c = G_LoadPlayer(sv->Filename);
        if (c == 0)
            g_player[myconnectindex].ps->gm = MODE_GAME;
        return !c;
    }
}

bool GameInterface::SaveGame(FSaveGameNode* sv)
{
    if (g_netServer || ud.multimode > 1)
    {
		quoteMgr.InitializeQuote(QUOTE_RESERVED4, "Multiplayer Saving Not Yet Supported");
        FTA(QUOTE_RESERVED4, g_player[myconnectindex].ps);
		return false;
    }
    else
    {

		videoNextPage();	// no idea if this is needed here.
        return G_SavePlayer(sv);
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

static int32_t ds_getcnt(const dataspec_t *spec)
{
    int cnt = -1;

    switch (spec->flags & DS_CNTMASK)
    {
        case 0: cnt = spec->cnt; break;
        case DS_CNT16: cnt = *((int16_t *)spec->cnt); break;
        case DS_CNT32: cnt = *((int32_t *)spec->cnt); break;
    }

    return cnt;
}

static inline void ds_get(const dataspec_t *spec, void **ptr, int32_t *cnt)
{
    *cnt = ds_getcnt(spec);
    *ptr = (spec->flags & DS_DYNAMIC) ? *((void **)spec->ptr) : spec->ptr;
}

// write state to file and/or to dump
static uint8_t *writespecdata(const dataspec_t *spec, FileWriter *fil, uint8_t *dump)
{
    for (; spec->flags != DS_END; spec++)
    {
        if (spec->flags & (DS_SAVEFN|DS_LOADFN))
        {
            if (spec->flags & DS_SAVEFN)
                (*(void (*)(void))spec->ptr)();
            continue;
        }

        if (!fil && (spec->flags & (DS_NOCHK|DS_CMP|DS_STRING)))
            continue;
        else if (spec->flags & DS_STRING)
        {
            fil->Write(spec->ptr, Bstrlen((const char *)spec->ptr));  // not null-terminated!
            continue;
        }

        void *  ptr;
        int32_t cnt;

        ds_get(spec, &ptr, &cnt);

        if (cnt < 0)
        {
            Printf("wsd: cnt=%d, f=0x%x.\n", cnt, spec->flags);
            continue;
        }

        if (!ptr || !cnt)
            continue;

        if (fil)
        {
			fil->Write(ptr, spec->size * cnt);
        }

        if (dump && (spec->flags & (DS_NOCHK|DS_CMP)) == 0)
        {
            Bmemcpy(dump, ptr, spec->size * cnt);
            dump += spec->size * cnt;
        }
    }
    return dump;
}

// let havedump=dumpvar&&*dumpvar
// (fil>=0 && havedump): first restore dump from file, then restore state from dump
// (fil<0 && havedump): only restore state from dump
// (fil>=0 && !havedump): only restore state from file
static int32_t readspecdata(const dataspec_t *spec, FileReader *fil, uint8_t **dumpvar)
{
    uint8_t *  dump = dumpvar ? *dumpvar : NULL;
    auto const sptr = spec;

    for (; spec->flags != DS_END; spec++)
    {
        if (fil == nullptr && spec->flags & (DS_NOCHK|DS_STRING|DS_CMP))  // we're updating
            continue;

        if (spec->flags & (DS_LOADFN|DS_SAVEFN))
        {
            if (spec->flags & DS_LOADFN)
                (*(void (*)())spec->ptr)();
            continue;
        }

        if (spec->flags & (DS_STRING|DS_CMP))  // DS_STRING and DS_CMP is for static data only
        {
            static char cmpstrbuf[32];
            int const siz  = (spec->flags & DS_STRING) ? Bstrlen((const char *)spec->ptr) : spec->size * spec->cnt;
            int const ksiz = fil->Read(cmpstrbuf, siz);

            if (ksiz != siz || Bmemcmp(spec->ptr, cmpstrbuf, siz))
            {
                Printf("rsd: spec=%s, idx=%d:\n", (char *)sptr->ptr, (int32_t)(spec-sptr));

                if (ksiz!=siz)
                    Printf("    file read returned %d, expected %d.\n", ksiz, siz);
                else
                    Printf("    sp->ptr and cmpstrbuf not identical!\n");

                return -1;
            }
            continue;
        }

        void *  ptr;
        int32_t cnt;

        ds_get(spec, &ptr, &cnt);

        if (cnt < 0)
        {
            Printf("rsd: cnt<0... wtf?\n");
            return -1;
        }

        if (!ptr || !cnt)
            continue;

        if (fil != nullptr && fil->isOpen())
        {
            auto const mem  = (dump && (spec->flags & DS_NOCHK) == 0) ? dump : (uint8_t *)ptr;
            int const  siz  = cnt * spec->size;
            int const  ksiz = fil->Read(mem, siz);

            if (ksiz != siz)
            {
                Printf("rsd: spec=%s, idx=%d, mem=%p\n", (char *)sptr->ptr, (int32_t)(spec - sptr), mem);
                Printf("     : read %d, expected %d!\n",
                           ksiz, siz);

                if (ksiz == -1)
                    Printf("     read: %s\n", strerror(errno));

                return -1;
            }
        }

        if (dump && (spec->flags & DS_NOCHK) == 0)
        {
            Bmemcpy(ptr, dump, spec->size * cnt);
            dump += spec->size * cnt;
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
#define CPSINGLEVAL(Datbits)                                              \
    if (VAL(Datbits, ptr) != VAL(Datbits, dump))                          \
    {                                                                     \
        WVAL(Datbits, retdiff) = WVAL(Datbits, dump) = VAL(Datbits, ptr); \
        *diffvar = retdiff + BYTES(Datbits);                              \
    }

    if (cnt == 1)
        switch (size)
        {
        case 8: CPSINGLEVAL(64); return;
        case 4: CPSINGLEVAL(32); return;
        case 2: CPSINGLEVAL(16); return;
        case 1: CPSINGLEVAL(8); return;
        }

#define CPELTS(Idxbits, Datbits)             \
    do                                       \
    {                                        \
        for (int i = 0; i < nelts; i++)      \
        {                                    \
            if (*p != *op)                   \
            {                                \
                *op = *p;                    \
                WVAL(Idxbits, retdiff) = i;  \
                retdiff += BYTES(Idxbits);   \
                WVAL(Datbits, retdiff) = *p; \
                retdiff += BYTES(Datbits);   \
            }                                \
            p++;                             \
            op++;                            \
        }                                    \
        WVAL(Idxbits, retdiff) = -1;         \
        retdiff += BYTES(Idxbits);           \
    } while (0)

#define CPDATA(Datbits)                                                  \
    do                                                                   \
    {                                                                    \
        auto p     = (UINT(Datbits) const *)ptr;                         \
        auto op    = (UINT(Datbits) *)dump;                              \
        int  nelts = tabledivide32_noinline(size * cnt, BYTES(Datbits)); \
        if (nelts > 65536)                                               \
            CPELTS(32, Datbits);                                         \
        else if (nelts > 256)                                            \
            CPELTS(16, Datbits);                                         \
        else                                                             \
            CPELTS(8, Datbits);                                          \
    } while (0)

    if (size == 8)
        CPDATA(64);
    else if ((size & 3) == 0)
        CPDATA(32);
    else if ((size & 1) == 0)
        CPDATA(16);
    else
        CPDATA(8);

    *diffvar = retdiff;

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
}

// get the number of elements to be monitored for changes
static int32_t getnumvar(const dataspec_t *spec)
{
    int n = 0;
    for (; spec->flags != DS_END; spec++)
        if (spec->flags & (DS_STRING|DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN))
            ++n;
    return n;
}

#define VALOFS(bits,p,ofs) (*(((UINT(bits) *)(p)) + (ofs)))


#undef VAL
#undef VALOFS
#undef BYTES
#undef UINT

// calculate size needed for dump
static uint32_t calcsz(const dataspec_t *spec)
{
    uint32_t dasiz = 0;

    for (; spec->flags != DS_END; spec++)
    {
        // DS_STRINGs are used as sync checks in the diffs but not in the dump
        if ((spec->flags & (DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN|DS_STRING)))
            continue;

        int const cnt = ds_getcnt(spec);

        if (cnt <= 0)
            continue;

        dasiz += cnt * spec->size;
    }

    return dasiz;
}

static void sv_postactordata();
static void sv_preanimateptrsave();
static void sv_postanimateptr();
static void sv_restsave();
static void sv_restload();
static void sv_rrrafog();

#define SVARDATALEN \
    ((sizeof(g_player[0].user_name)+sizeof(g_player[0].pcolor)+sizeof(g_player[0].pteam) \
      +sizeof(g_player[0].frags)+sizeof(struct player_struct))*MAXPLAYERS)

static uint8_t savegame_restdata[SVARDATALEN];

static char svgm_udnetw_string [] = "blK:udnt";
static const dataspec_t svgm_udnetw[] =
{
    { DS_STRING, (void *)svgm_udnetw_string, 0, 1 },
    { 0, &ud.multimode, sizeof(ud.multimode), 1 },
    { 0, &numplayersprites, sizeof(numplayersprites), 1 },
    { 0, &po, sizeof(po), 1 },

    { DS_NOCHK, &ud.player_skill, sizeof(ud.player_skill), 1 },

    { DS_NOCHK, &ud.from_bonus, sizeof(ud.from_bonus), 1 },
    { DS_NOCHK, &ud.secretlevel, sizeof(ud.secretlevel), 1 },
    { DS_NOCHK, &ud.respawn_monsters, sizeof(ud.respawn_monsters), 1 },
    { DS_NOCHK, &ud.respawn_items, sizeof(ud.respawn_items), 1 },
    { DS_NOCHK, &ud.respawn_inventory, sizeof(ud.respawn_inventory), 1 },
    { 0, &ud.god, sizeof(ud.god), 1 },
    { DS_NOCHK, &ud.monsters_off, sizeof(ud.monsters_off), 1 },
    { DS_NOCHK, &ud.last_level, sizeof(ud.last_level), 1 },
    { 0, &ud.eog, sizeof(ud.eog), 1 },
    { DS_NOCHK, &ud.coop, sizeof(ud.coop), 1 },
    { DS_NOCHK, &ud.marker, sizeof(ud.marker), 1 },
    { DS_NOCHK, &ud.ffire, sizeof(ud.ffire), 1 },
    { 0, &ud.pause_on, sizeof(ud.pause_on), 1 },
    { 0, connectpoint2, sizeof(connectpoint2), 1 },
    { 0, &randomseed, sizeof(randomseed), 1 },
    { 0, &global_random, sizeof(global_random), 1 },
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
    { DS_NOCHK, &g_cyclerCnt, sizeof(g_cyclerCnt), 1 },
    { DS_CNT(g_cyclerCnt), &g_cyclers[0][0], sizeof(g_cyclers[0]), (intptr_t)&g_cyclerCnt },
    { DS_NOCHK, &g_animWallCnt, sizeof(g_animWallCnt), 1 },
    { DS_CNT(g_animWallCnt), &animwall, sizeof(animwall[0]), (intptr_t)&g_animWallCnt },
    { DS_NOCHK, &mirrorcnt, sizeof(mirrorcnt), 1 },
    { DS_NOCHK, &mirrorwall[0], sizeof(mirrorwall[0]), ARRAY_SIZE(mirrorwall) },
    { DS_NOCHK, &mirrorsector[0], sizeof(mirrorsector[0]), ARRAY_SIZE(mirrorsector) },
    { 0, &everyothertime, sizeof(everyothertime), 1 },
    { DS_END, 0, 0, 0 }
};

static char svgm_script_string [] = "blK:scri";
static const dataspec_t svgm_script[] =
{
    { DS_STRING, (void *)svgm_script_string, 0, 1 },
    { 0, &hittype[0], sizeof(weaponhit), MAXSPRITES },
    { DS_SAVEFN|DS_LOADFN, (void *)&sv_postactordata, 0, 1 },
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
    { 0, &camsprite, sizeof(camsprite), 1 },
    { 0, &g_origins[0], sizeof(g_origins[0]), ARRAY_SIZE(g_origins) },
    { 0, &g_spriteDeleteQueuePos, sizeof(g_spriteDeleteQueuePos), 1 },
    { DS_NOCHK, &spriteqamount, sizeof(spriteqamount), 1 },
    { DS_CNT(spriteqamount), &SpriteDeletionQueue[0], sizeof(int16_t), (intptr_t)&spriteqamount },
    { DS_NOCHK, &numclouds, sizeof(numclouds), 1 },
    { 0, &clouds[0], sizeof(clouds), 1 },
    { 0, &cloudx, sizeof(cloudx), 1 },
    { 0, &cloudy, sizeof(cloudy), 1 },
    { 0, &g_pskyidx, sizeof(g_pskyidx), 1 },  // DS_NOCHK?
    { 0, &g_earthquakeTime, sizeof(g_earthquakeTime), 1 },

    // RR stuff
    { 0, &g_spriteExtra[0], sizeof(g_spriteExtra[0]), MAXSPRITES },
    { 0, &g_sectorExtra[0], sizeof(g_sectorExtra[0]), MAXSECTORS },

    { 0, &shadedsector[0], sizeof(shadedsector[0]), MAXSECTORS },

    { 0, &ambientfx, sizeof(ambientfx), 1 },
    { 0, &ambienthitag[0], sizeof(ambienthitag[0]), ARRAY_SIZE(ambienthitag) },
    { 0, &ambientlotag[0], sizeof(ambientlotag[0]), ARRAY_SIZE(ambientlotag) },
    
    { 0, &g_ufoSpawn, sizeof(g_ufoSpawn), 1 },
    { 0, &g_ufoCnt, sizeof(g_ufoCnt), 1 },
    { 0, &g_hulkSpawn, sizeof(g_hulkSpawn), 1 },
    { 0, &g_lastLevel, sizeof(g_lastLevel), 1 },

    { 0, &geosector[0], sizeof(geosector[0]), ARRAY_SIZE(geosector) },
    { 0, &geosectorwarp[0], sizeof(geosectorwarp[0]), ARRAY_SIZE(geosectorwarp) },
    { 0, &geox[0], sizeof(geox[0]), ARRAY_SIZE(geox) },
    { 0, &geoy[0], sizeof(geoy[0]), ARRAY_SIZE(geoy) },
    { 0, &geosectorwarp2[0], sizeof(geosectorwarp2[0]), ARRAY_SIZE(geosectorwarp2) },
    { 0, &geox2[0], sizeof(geox2[0]), ARRAY_SIZE(geox2) },
    { 0, &geoy2[0], sizeof(geoy2[0]), ARRAY_SIZE(geoy2) },
    { 0, &geocnt, sizeof(geocnt), 1 },

    { 0, &WindTime, sizeof(WindTime), 1 },
    { 0, &WindDir, sizeof(WindDir), 1 },
    { 0, &fakebubba_spawn, sizeof(fakebubba_spawn), 1 },
    { 0, &mamaspawn_count, sizeof(mamaspawn_count), 1 },
    { 0, &banjosound, sizeof(banjosound), 1 },
    { 0, &g_bellTime, sizeof(g_bellTime), 1 },
    { 0, &BellSprite, sizeof(BellSprite), 1 },

    { 0, &enemysizecheat, sizeof(enemysizecheat), 1 },
    { 0, &ufospawnsminion, sizeof(ufospawnsminion), 1 },
    { 0, &pistonsound, sizeof(pistonsound), 1 },
    { 0, &chickenphase, sizeof(chickenphase), 1 },
    { 0, &RRRA_ExitedLevel, sizeof(RRRA_ExitedLevel), 1 },
    { 0, &fogactive, sizeof(fogactive), 1 },
    { DS_LOADFN, (void *)sv_rrrafog, 0, 1 },

    { DS_SAVEFN, (void *)&sv_restsave, 0, 1 },
    { 0, savegame_restdata, 1, sizeof(savegame_restdata) },  // sz/cnt swapped for kdfread
    { DS_LOADFN, (void *)&sv_restload, 0, 1 },

    { DS_STRING, (void *)svgm_end_string, 0, 1 },
    { DS_END, 0, 0, 0 }
};

static dataspec_gv_t *svgm_vars=NULL;
static uint8_t *dosaveplayer2(FileWriter *fil, uint8_t *mem);
static int32_t doloadplayer2(FileReader &fil, uint8_t **memptr);
static void postloadplayer(int32_t savegamep);

// SVGM snapshot system
static uint32_t svsnapsiz;
static uint8_t *svsnapshot;
static uint8_t *svinitsnap;
static uint32_t svdiffsiz;
static uint8_t *svdiff;

#include "gamedef.h"

#define SV_SKIPMASK (GAMEVAR_READONLY | GAMEVAR_PTR_MASK)

static char svgm_vars_string [] = "blK:vars";
// setup gamevar data spec for snapshotting and diffing... gamevars must be loaded when called
static void sv_makevarspec()
{
}

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
int32_t sv_saveandmakesnapshot(FileWriter &fil, int8_t spot, bool isAutoSave)
{
    savehead_t h;

    // calculate total snapshot size
    sv_makevarspec();
    svsnapsiz = calcsz((const dataspec_t *)svgm_vars);
    svsnapsiz += calcsz(svgm_udnetw) + calcsz(svgm_secwsp) + calcsz(svgm_script) + calcsz(svgm_anmisc);


    // create header
    Bmemcpy(h.headerstr, "DERSAVEGAME", 11);
    h.majorver = SV_MAJOR_VER;
    h.minorver = SV_MINOR_VER;
    h.ptrsize  = sizeof(intptr_t);

    if (isAutoSave)
        h.ptrsize |= 1u << 7u;

    h.bytever = 0;
    h.userbytever  = 0;
    h.scriptcrc    = 0;

    h.reccnt  = 0;
    h.snapsiz = svsnapsiz;

    // the following is kinda redundant, but we save it here to be able to quickly fetch
    // it in a savegame header read

    h.numplayers = ud.multimode;
    h.volnum     = 0;
    h.levnum     = 0;
    h.skill      = ud.player_skill;

    if (spot >= 0)
    {
        // savegame
		auto fw = WriteSavegameChunk("header.dat");
		fw->Write(&h, sizeof(savehead_t));
	}


    // write header

    if (spot >= 0)
    {
        // savegame
        dosaveplayer2(&fil, NULL);
    }

    return 0;
}

// if file is not an EDuke32 savegame/demo, h->headerstr will be all zeros
int32_t sv_loadheader(FileReader &fill, int32_t spot, savehead_t *h)
{
	FileReader filc;
	FileReader* filp = &fill;
    int32_t havedemo = (spot < 0);
	if (!havedemo)
	{
		filc = ReadSavegameChunk("header.dat");
		filp = &filc;
	}

    if (filp->Read(h, sizeof(savehead_t)) != sizeof(savehead_t))
    {
        Printf("%s %d header corrupt.\n", havedemo ? "Demo":"Savegame", havedemo ? -spot : spot);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -1;
    }

    if (Bmemcmp(h->headerstr, "DERSAVEGAME", 11)
       )
    {
        char headerCstr[sizeof(h->headerstr) + 1];
        Bmemcpy(headerCstr, h->headerstr, sizeof(h->headerstr));
        headerCstr[sizeof(h->headerstr)] = '\0';
        Printf("%s %d header reads \"%s\", expected \"DERSAVEGAME\".\n",
                   havedemo ? "Demo":"Savegame", havedemo ? -spot : spot, headerCstr);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -2;
    }

    if (h->majorver != SV_MAJOR_VER || h->minorver != SV_MINOR_VER || h->bytever != 0 || h->userbytever != 0 || ScriptCode.Size())
    {
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

    if (h->getPtrSize() != sizeof(intptr_t))
    {
#ifndef DEBUGGINGAIDS
        if (havedemo)
#endif
            Printf("File incompatible. Expected pointer size %d, found %d\n",
                       (int32_t)sizeof(intptr_t), h->getPtrSize());

        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -4;
    }

    return 0;
}

int32_t sv_loadsnapshot(FileReader &fil, int32_t spot, savehead_t *h)
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
    Printf("sv_loadsnapshot: snapshot size: %d bytes.\n", h->snapsiz);
#endif

    if (spot >= 0)
    {
        // savegame
        i = doloadplayer2(fil, NULL);
        if (i)
        {
            Printf("sv_loadsnapshot: doloadplayer2() returned %d.\n", i);
            return 5;
        }
    }
    else
    {
        svsnapsiz = h->snapsiz;

        SV_AllocSnap(1);

        p = svsnapshot;
        i = doloadplayer2(fil, &p);
        if (i)
        {
            Printf("sv_loadsnapshot: doloadplayer2() returned %d.\n", i);
            sv_freemem();
            return 5;
        }

        if (p != svsnapshot+svsnapsiz)
        {
            Printf("sv_loadsnapshot: internal error: p-(snapshot end)=%d!\n",
                       (int32_t)(p-(svsnapshot+svsnapsiz)));
            sv_freemem();
            return 6;
        }

        Bmemcpy(svinitsnap, svsnapshot, svsnapsiz);
    }

    postloadplayer((spot >= 0));

    return 0;
}



// SVGM data description
static void sv_postudload()
{
#if 1
    ud.m_player_skill      = ud.player_skill;
    ud.m_respawn_monsters  = ud.respawn_monsters;
    ud.m_respawn_items     = ud.respawn_items;
    ud.m_respawn_inventory = ud.respawn_inventory;
    ud.m_monsters_off      = ud.monsters_off;
    m_coop              = ud.coop;
    m_marker            = ud.marker;
    m_ffire             = ud.ffire;
#endif
}
//static int32_t lockclock_dummy;


static void sv_postactordata()
{
#ifdef POLYMER
    for (auto & i : actor)
    {
        i.lightptr = NULL;
        i.lightId = -1;
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
static void sv_restsave()
{
    uint8_t *    mem = savegame_restdata;
    struct player_struct dummy_ps;

    Bmemset(&dummy_ps, 0, sizeof(struct player_struct));

#define CPDAT(ptr,sz) do { Bmemcpy(mem, ptr, sz), mem+=sz ; } while (0)
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        CPDAT(g_player[i].ps ? g_player[i].ps : &dummy_ps, sizeof(struct player_struct));
    }
    
    Bassert((savegame_restdata + SVARDATALEN) - mem == 0);
#undef CPDAT
}
static void sv_restload()
{
    uint8_t *    mem = savegame_restdata;
    struct player_struct dummy_ps;

#define CPDAT(ptr,sz) Bmemcpy(ptr, mem, sz), mem+=sz
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        CPDAT(g_player[i].ps ? g_player[i].ps : &dummy_ps, sizeof(struct player_struct));
    }
#undef CPDAT

    if (g_player[myconnectindex].ps)
        g_player[myconnectindex].ps->auto_aim = cl_autoaim;
}

#ifdef DEBUGGINGAIDS
# define PRINTSIZE(name) do { if (mem) Printf(name ": %d\n", (int32_t)(mem-tmem)); \
        Printf(name ": %d ms\n", timerGetTicks()-t); t=timerGetTicks(); tmem=mem; } while (0)
#else
# define PRINTSIZE(name) do { } while (0)
#endif

static uint8_t *dosaveplayer2(FileWriter *fil, uint8_t *mem)
{
    mem=writespecdata(svgm_udnetw, fil, mem);  // user settings, players & net
    PRINTSIZE("ud");
    mem=writespecdata(svgm_secwsp, fil, mem);  // sector, wall, sprite
    PRINTSIZE("sws");
    mem=writespecdata(svgm_script, fil, mem);  // script
    PRINTSIZE("script");
    mem=writespecdata(svgm_anmisc, fil, mem);  // animates, quotes & misc.
    PRINTSIZE("animisc");
    //Gv_WriteSave(*fil);  // gamevars
    mem=writespecdata((const dataspec_t *)svgm_vars, 0, mem);
    PRINTSIZE("vars");

    return mem;
}

static int32_t doloadplayer2(FileReader &fil, uint8_t **memptr)
{
    uint8_t *mem = memptr ? *memptr : NULL;
    if (readspecdata(svgm_udnetw, &fil, &mem)) return -2;
    PRINTSIZE("ud");
    if (readspecdata(svgm_secwsp, &fil, &mem)) return -4;
    PRINTSIZE("sws");
    if (readspecdata(svgm_script, &fil, &mem)) return -5;
    PRINTSIZE("script");
    if (readspecdata(svgm_anmisc, &fil, &mem)) return -6;
    PRINTSIZE("animisc");

    //if ((i = Gv_ReadSave(fil))) return i;

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

    if (memptr)
        *memptr = mem;
    return 0;
}

int32_t sv_updatestate(int32_t frominit)
{
    uint8_t *p = svsnapshot, *pbeg=p;

    if (frominit)
        Bmemcpy(svsnapshot, svinitsnap, svsnapsiz);

    if (readspecdata(svgm_udnetw, nullptr, &p)) return -2;
    if (readspecdata(svgm_secwsp, nullptr, &p)) return -4;
    if (readspecdata(svgm_script, nullptr, &p)) return -5;
    if (readspecdata(svgm_anmisc, nullptr, &p)) return -6;
    if (readspecdata((const dataspec_t *)svgm_vars, nullptr, &p)) return -8;

    if (p != pbeg+svsnapsiz)
    {
        Printf("sv_updatestate: ptr-(snapshot end)=%d\n", (int32_t)(p-(pbeg+svsnapsiz)));
        return -9;
    }

    if (frominit)
        postloadplayer(0);
#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_resetlights();  // must do it after polymer_loadboard() !!!
#endif

    return 0;
}

static void sv_rrrafog()
{
    setmapfog(fogactive ? 2 : 0);
}

static void postloadplayer(int32_t savegamep)
{
    int32_t i;

    //1
    if (g_player[myconnectindex].ps->over_shoulder_on != 0)
    {
        cameradist = 0;
        cameraclock = 0;
        g_player[myconnectindex].ps->over_shoulder_on = 1;
    }

    //2
    screenpeek = myconnectindex;

    //2.5
    if (savegamep)
    {
        Bmemset(gotpic, 0, sizeof(gotpic));
        Mus_ResumeSaved();
        Mus_SetPaused(false);

        g_player[myconnectindex].ps->gm = MODE_GAME;
        ud.recstat = 0;

        if (g_player[myconnectindex].ps->jetpack_on)
            A_PlaySound(DUKE_JETPACK_IDLE, g_player[myconnectindex].ps->i);
    }

    //3
    setpal(g_player[myconnectindex].ps);

    //4
    if (savegamep)
    {
        for (SPRITES_OF(STAT_FX, i))
            if (sprite[i].picnum == MUSICANDSFX)
            {
                hittype[i].temp_data[1] = SoundEnabled();
                hittype[i].temp_data[0] = 0;
            }

        FX_SetReverb(0);
    }

    //5
    G_ResetInterpolations();

    //6
    show_shareware = 0;
    if (savegamep)
        everyothertime = 0;

    // ----------

    //7.5
    if (savegamep)
    {
        ready2send = 1;
        clearfifo();
        Net_WaitForEverybody();
    }

    //8
    // if (savegamep)  ?
    resettimevars();

#ifdef USE_STRUCT_TRACKERS
    Bmemset(sectorchanged, 0, sizeof(sectorchanged));
    Bmemset(spritechanged, 0, sizeof(spritechanged));
    Bmemset(wallchanged, 0, sizeof(wallchanged));
#endif

#ifdef POLYMER
    //9
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_loadboard();

    // this light pointer nulling needs to be outside the videoGetRenderMode check
    // because we might be loading the savegame using another renderer but
    // change to Polymer later
    for (i=0; i<MAXSPRITES; i++)
    {
        actor[i].lightptr = NULL;
        actor[i].lightId = -1;
    }
#endif
    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].ps->drug_timer = 0;

    G_InitRRRASkies();
}

////////// END GENERIC SAVING/LOADING SYSTEM //////////


END_DUKE_NS
