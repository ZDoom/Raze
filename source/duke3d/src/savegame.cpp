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

#include "vfs.h"

static OutputFileCounter savecounter;

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
uint16_t g_nummenusaves;

static menusave_t * g_internalsaves;
static uint16_t g_numinternalsaves;

static void ReadSaveGameHeaders_CACHE1D(CACHE1D_FIND_REC *f)
{
    savehead_t h;

    for (; f != nullptr; f = f->next)
    {
        char const * fn = f->name;
        buildvfs_kfd fil = kopen4loadfrommod(fn, 0);
        if (fil == buildvfs_kfd_invalid)
            continue;

        menusave_t & msv = g_internalsaves[g_numinternalsaves];

        msv.brief.isExt = 0;

        int32_t k = sv_loadheader(fil, 0, &h);
        if (k)
        {
            if (k < 0)
                msv.isUnreadable = 1;
            else
            {
                if (FURY)
                {
                    char extfn[BMAX_PATH];
                    snprintf(extfn, ARRAY_SIZE(extfn), "%s.ext", fn);
                    buildvfs_kfd extfil = kopen4loadfrommod(extfn, 0);
                    if (extfil != buildvfs_kfd_invalid)
                    {
                        msv.brief.isExt = 1;
                        kclose(extfil);
                    }
                }
            }
            msv.isOldVer = 1;
        }
        else
            msv.isOldVer = 0;

        msv.isAutoSave = h.isAutoSave();

        strncpy(msv.brief.path, fn, ARRAY_SIZE(msv.brief.path));
        ++g_numinternalsaves;

        if (k >= 0 && h.savename[0] != '\0')
        {
            memcpy(msv.brief.name, h.savename, ARRAY_SIZE(msv.brief.name));
        }
        else
            msv.isUnreadable = 1;

        kclose(fil);
    }
}

static int countcache1dfind(CACHE1D_FIND_REC *f)
{
    int x = 0;
    for (; f != nullptr; f = f->next)
        ++x;
    return x;
}

static void ReadSaveGameHeaders_Internal(void)
{
    static char const DefaultPath[] = "/", SavePattern[] = "*.esv";

    CACHE1D_FIND_REC *findfiles_default = klistpath(DefaultPath, SavePattern, CACHE1D_FIND_FILE);

    // potentially overallocating but programmatically simple
    int const numfiles = countcache1dfind(findfiles_default);
    size_t const internalsavesize = sizeof(menusave_t) * numfiles;

    g_internalsaves = (menusave_t *)Xrealloc(g_internalsaves, internalsavesize);

    for (int x = 0; x < numfiles; ++x)
        g_internalsaves[x].clear();

    g_numinternalsaves = 0;
    ReadSaveGameHeaders_CACHE1D(findfiles_default);
    klistfree(findfiles_default);

    g_nummenusaves = 0;
    for (int x = g_numinternalsaves-1; x >= 0; --x)
    {
        menusave_t & msv = g_internalsaves[x];
        if (!msv.isUnreadable)
        {
            ++g_nummenusaves;
        }
    }
    size_t const menusavesize = sizeof(menusave_t) * g_nummenusaves;

    g_menusaves = (menusave_t *)Xrealloc(g_menusaves, menusavesize);

    for (int x = 0; x < g_nummenusaves; ++x)
        g_menusaves[x].clear();

    for (int x = g_numinternalsaves-1, y = 0; x >= 0; --x)
    {
        menusave_t & msv = g_internalsaves[x];
        if (!msv.isUnreadable)
        {
            g_menusaves[y++] = msv;
        }
    }

    for (int x = g_numinternalsaves-1; x >= 0; --x)
    {
        char const * const path = g_internalsaves[x].brief.path;
        int const pathlen = Bstrlen(path);
        if (pathlen < 12)
            continue;
        char const * const fn = path + (pathlen-12);
        if (fn[0] == 's' && fn[1] == 'a' && fn[2] == 'v' && fn[3] == 'e' &&
            isdigit(fn[4]) && isdigit(fn[5]) && isdigit(fn[6]) && isdigit(fn[7]))
        {
            char number[5];
            memcpy(number, fn+4, 4);
            number[4] = '\0';
            savecounter.count = Batoi(number)+1;
            break;
        }
    }
}

void ReadSaveGameHeaders(void)
{
    ReadSaveGameHeaders_Internal();

    if (!ud.autosavedeletion)
        return;

    bool didDelete = false;
    int numautosaves = 0;
    for (int x = 0; x < g_nummenusaves; ++x)
    {
        menusave_t & msv = g_menusaves[x];
        if (!msv.isAutoSave)
            continue;
        if (numautosaves >= ud.maxautosaves)
        {
            G_DeleteSave(msv.brief);
            didDelete = true;
        }
        ++numautosaves;
    }

    if (didDelete)
        ReadSaveGameHeaders_Internal();
}

int32_t G_LoadSaveHeaderNew(char const *fn, savehead_t *saveh)
{
    buildvfs_kfd fil = kopen4loadfrommod(fn, 0);
    if (fil == buildvfs_kfd_invalid)
        return -1;

    int32_t i = sv_loadheader(fil, 0, saveh);
    if (i < 0)
        goto corrupt;

    int32_t screenshotofs;
    if (kread(fil, &screenshotofs, 4) != 4)
        goto corrupt;

    walock[TILE_LOADSHOT] = 255;
    if (waloff[TILE_LOADSHOT] == 0)
        cacheAllocateBlock(&waloff[TILE_LOADSHOT], 320*200, &walock[TILE_LOADSHOT]);
    tilesiz[TILE_LOADSHOT].x = 200;
    tilesiz[TILE_LOADSHOT].y = 320;
    if (screenshotofs)
    {
        if (kdfread_LZ4((char *)waloff[TILE_LOADSHOT], 320, 200, fil) != 200)
        {
            OSD_Printf("G_LoadSaveHeaderNew(): failed reading screenshot in \"%s\"\n", fn);
            goto corrupt;
        }

#if 0
        // debug code to dump the screenshot
        char scrbuf[BMAX_PATH];
        if (G_ModDirSnprintf(scrbuf, sizeof(scrbuf), "%s.raw", fn) == 0)
        {
            buildvfs_FILE scrfil = buildvfs_fopen_write(scrbuf);
            buildvfs_fwrite((char *)waloff[TILE_LOADSHOT], 320, 200, scrfil);
            buildvfs_fclose(scrfil);
        }
#endif
    }
    else
    {
        Bmemset((char *)waloff[TILE_LOADSHOT], 0, 320*200);
    }
    tileInvalidate(TILE_LOADSHOT, 0, 255);

    kclose(fil);
    return 0;

corrupt:
    kclose(fil);
    return 1;
}


static void sv_postudload();

// hack
static int different_user_map;

#include "sjson.h"

// XXX: keyboard input 'blocked' after load fail? (at least ESC?)
int32_t G_LoadPlayer(savebrief_t & sv)
{
    if (sv.isExt)
    {
        int volume = -1;
        int level = -1;
        int skill = -1;

        buildvfs_kfd const fil = kopen4loadfrommod(sv.path, 0);

        if (fil != buildvfs_kfd_invalid)
        {
            savehead_t h;
            int status = sv_loadheader(fil, 0, &h);
            if (status >= 0)
            {
                volume = h.volnum;
                level = h.levnum;
                skill = h.skill;
            }

            kclose(fil);
        }

        char extfn[BMAX_PATH];
        snprintf(extfn, ARRAY_SIZE(extfn), "%s.ext", sv.path);
        buildvfs_kfd extfil = kopen4loadfrommod(extfn, 0);
        if (extfil == buildvfs_kfd_invalid)
        {
            return -1;
        }

        int32_t len = kfilelength(extfil);
        auto text = (char *)Xmalloc(len+1);
        text[len] = '\0';

        if (kread_and_test(extfil, text, len))
        {
            kclose(extfil);
            Xfree(text);
            return -1;
        }

        kclose(extfil);


        sjson_context * ctx = sjson_create_context(0, 0, NULL);
        sjson_node * root = sjson_decode(ctx, text);

        Xfree(text);

        if (volume == -1)
            volume = sjson_get_int(root, "volume", volume);
        if (level == -1)
            level = sjson_get_int(root, "level", level);
        if (skill == -1)
            skill = sjson_get_int(root, "skill", skill);

        if (volume == -1 || level == -1 || skill == -1)
        {
            sjson_destroy_context(ctx);
            return -1;
        }

        sjson_node * players = sjson_find_member(root, "players");

        int numplayers = sjson_child_count(players);

        if (numplayers != ud.multimode)
        {
            P_DoQuote(QUOTE_SAVE_BAD_PLAYERS, g_player[myconnectindex].ps);

            sjson_destroy_context(ctx);
            return 1;
        }


        ud.returnvar[0] = level;
        volume = VM_OnEventWithReturn(EVENT_VALIDATESTART, g_player[myconnectindex].ps->i, myconnectindex, volume);
        level = ud.returnvar[0];


        {
            // CODEDUP from non-isExt branch, with simplifying assumptions

            VM_OnEvent(EVENT_PRELOADGAME, g_player[screenpeek].ps->i, screenpeek);

            ud.multimode = numplayers;

            Net_WaitForServer();

            FX_StopAllSounds();
            S_ClearSoundLocks();

            ud.m_volume_number = volume;
            ud.m_level_number = level;
            ud.m_player_skill = skill;

            boardfilename[0] = '\0';

            int const mapIdx = volume*MAXLEVELS + level;

            if (boardfilename[0])
                Bstrcpy(currentboardfilename, boardfilename);
            else if (g_mapInfo[mapIdx].filename)
                Bstrcpy(currentboardfilename, g_mapInfo[mapIdx].filename);

            if (currentboardfilename[0])
            {
                artSetupMapArt(currentboardfilename);
                append_ext_UNSAFE(currentboardfilename, ".mhk");
                engineLoadMHK(currentboardfilename);
            }

            currentboardfilename[0] = '\0';

            // G_NewGame_EnterLevel();
        }

        {
            // CODEDUP from G_NewGame

            auto & p0 = *g_player[0].ps;

            ready2send = 0;

            ud.from_bonus    = 0;
            ud.last_level    = -1;
            ud.level_number  = level;
            ud.player_skill  = skill;
            ud.secretlevel   = 0;
            ud.skill_voice   = -1;
            ud.volume_number = volume;

            g_lastAutoSaveArbitraryID = -1;

#ifdef EDUKE32_TOUCH_DEVICES
            p0.zoom = 360;
#else
            p0.zoom = 768;
#endif
            p0.gm = 0;

            Menu_Close(0);

#if !defined LUNATIC
            Gv_ResetVars();
            Gv_InitWeaponPointers();
            Gv_RefreshPointers();
#endif
            Gv_ResetSystemDefaults();

            for (int i=0; i < (MAXVOLUMES*MAXLEVELS); i++)
                G_FreeMapState(i);

            if (ud.m_coop != 1)
                p0.last_weapon = -1;

            display_mirror = 0;
        }

        int p = 0;
        for (sjson_node * player = sjson_first_child(players); player != nullptr; player = player->next)
        {
            playerdata_t * playerData = &g_player[p];
            DukePlayer_t * ps = playerData->ps;
            auto pSprite = &sprite[ps->i];

            pSprite->extra = sjson_get_int(player, "extra", -1);
            ps->max_player_health = sjson_get_int(player, "max_player_health", -1);

            sjson_node * gotweapon = sjson_find_member(player, "gotweapon");
            int w_end = min<int>(MAX_WEAPONS, sjson_child_count(gotweapon));
            ps->gotweapon = 0;
            for (int w = 0; w < w_end; ++w)
            {
                sjson_node * ele = sjson_find_element(gotweapon, w);
                if (ele->tag == SJSON_BOOL && ele->bool_)
                    ps->gotweapon |= 1<<w;
            }

            /* bool flag_ammo_amount = */ sjson_get_int16s(ps->ammo_amount, MAX_WEAPONS, player, "ammo_amount");
            /* bool flag_max_ammo_amount = */ sjson_get_int16s(ps->max_ammo_amount, MAX_WEAPONS, player, "max_ammo_amount");
            /* bool flag_inv_amount = */ sjson_get_int16s(ps->inv_amount, GET_MAX, player, "inv_amount");

            ps->max_shield_amount = sjson_get_int(player, "max_shield_amount", -1);

            ps->curr_weapon = sjson_get_int(player, "curr_weapon", -1);
            ps->subweapon = sjson_get_int(player, "subweapon", -1);
            ps->inven_icon = sjson_get_int(player, "inven_icon", -1);

            sjson_node * vars = sjson_find_member(player, "vars");

            for (int j=0; j<g_gameVarCount; j++)
            {
                gamevar_t & var = aGameVars[j];

                if (!(var.flags & GAMEVAR_SERIALIZE))
                    continue;

                if ((var.flags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR)) != GAMEVAR_PERPLAYER)
                    continue;

                Gv_SetVar(j, sjson_get_int(vars, var.szLabel, var.defaultValue), ps->i, p);
            }

            ++p;
        }

        {
            sjson_node * vars = sjson_find_member(root, "vars");

            for (int j=0; j<g_gameVarCount; j++)
            {
                gamevar_t & var = aGameVars[j];

                if (!(var.flags & GAMEVAR_SERIALIZE))
                    continue;

                if (var.flags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR))
                    continue;

                Gv_SetVar(j, sjson_get_int(vars, var.szLabel, var.defaultValue));
            }
        }

        sjson_destroy_context(ctx);


        if (G_EnterLevel(MODE_GAME|MODE_EOL))
            G_BackToMenu();


        // postloadplayer(1);

        // sv_postudload();


        VM_OnEvent(EVENT_LOADGAME, g_player[screenpeek].ps->i, screenpeek);

        return 0;
    }

    buildvfs_kfd const fil = kopen4loadfrommod(sv.path, 0);

    if (fil == buildvfs_kfd_invalid)
        return -1;

    ready2send = 0;

    savehead_t h;
    int status = sv_loadheader(fil, 0, &h);

    if (status < 0 || h.numplayers != ud.multimode)
    {
        if (status == -4 || status == -3 || status == 1)
            P_DoQuote(QUOTE_SAVE_BAD_VERSION, g_player[myconnectindex].ps);
        else if (h.numplayers != ud.multimode)
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
        videoNextPage();
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
    different_user_map = Bstrcmp(boardfilename, h.boardfn);
    Bmemcpy(boardfilename, h.boardfn, sizeof(boardfilename));

    int const mapIdx = h.volnum*MAXLEVELS + h.levnum;

    if (boardfilename[0])
        Bstrcpy(currentboardfilename, boardfilename);
    else if (g_mapInfo[mapIdx].filename)
        Bstrcpy(currentboardfilename, g_mapInfo[mapIdx].filename);

    if (currentboardfilename[0])
    {
        artSetupMapArt(currentboardfilename);
        append_ext_UNSAFE(currentboardfilename, ".mhk");
        engineLoadMHK(currentboardfilename);
    }

    Bmemcpy(currentboardfilename, boardfilename, BMAX_PATH);

    if (status == 2)
        G_NewGame_EnterLevel();
    else if ((status = sv_loadsnapshot(fil, 0, &h)))  // read the rest...
    {
        // in theory, we could load into an initial dump first and trivially
        // recover if things go wrong...
        Bsprintf(tempbuf, "Loading save game file \"%s\" failed (code %d), cannot recover.", sv.path, status);
        G_GameExit(tempbuf);
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
    g_timers.totalclock     = (int32_t) totalclock;
    g_timers.totalclocklock = (int32_t) totalclocklock;
    g_timers.ototalclock    = (int32_t) ototalclock;
    g_timers.lockclock      = (int32_t) lockclock;
}

static void G_RestoreTimers(void)
{
    timerUpdate();

    totalclock     = g_timers.totalclock;
    totalclocklock = g_timers.totalclocklock;
    ototalclock    = g_timers.ototalclock;
    lockclock      = g_timers.lockclock;
}

//////////

void G_DeleteSave(savebrief_t const & sv)
{
    if (!sv.isValid())
        return;

    char temp[BMAX_PATH];

    if (G_ModDirSnprintf(temp, sizeof(temp), "%s", sv.path))
    {
        OSD_Printf("G_SavePlayer: file name \"%s\" too long\n", sv.path);
        return;
    }

    buildvfs_unlink(temp);
    Bstrcat(temp, ".ext");
    buildvfs_unlink(temp);
}

void G_DeleteOldSaves(void)
{
    ReadSaveGameHeaders();

    for (int x = 0; x < g_numinternalsaves; ++x)
    {
        menusave_t const & msv = g_internalsaves[x];
        if (msv.isOldVer || msv.isUnreadable)
            G_DeleteSave(msv.brief);
    }
}

uint16_t G_CountOldSaves(void)
{
    ReadSaveGameHeaders();

    int bad = 0;
    for (int x = 0; x < g_numinternalsaves; ++x)
    {
        menusave_t const & msv = g_internalsaves[x];
        if (msv.isOldVer || msv.isUnreadable)
            ++bad;
    }

    return bad;
}

int32_t G_SavePlayer(savebrief_t & sv, bool isAutoSave)
{
#ifdef __ANDROID__
    G_SavePalette();
#endif

    G_SaveTimers();

    Net_WaitForServer();
    ready2send = 0;

    char fn[BMAX_PATH];

    errno = 0;
    buildvfs_FILE fil;

    if (sv.isValid())
    {
        if (G_ModDirSnprintf(fn, sizeof(fn), "%s", sv.path))
        {
            OSD_Printf("G_SavePlayer: file name \"%s\" too long\n", sv.path);
            goto saveproblem;
        }
        fil = buildvfs_fopen_write(fn);
    }
    else
    {
        static char const SaveName[] = "save0000.esv";
        int const len = G_ModDirSnprintfLite(fn, ARRAY_SIZE(fn), SaveName);
        if (len >= ARRAY_SSIZE(fn)-1)
        {
            OSD_Printf("G_SavePlayer: could not form automatic save path\n");
            goto saveproblem;
        }
        char * zeros = fn + (len-8);
        fil = savecounter.opennextfile(fn, zeros);
        savecounter.count++;
        // don't copy the mod dir into sv.path
        Bstrcpy(sv.path, fn + (len-(ARRAY_SIZE(SaveName)-1)));
    }

    if (!fil)
    {
        OSD_Printf("G_SavePlayer: failed opening \"%s\" for writing: %s\n",
                   fn, strerror(errno));
        goto saveproblem;
    }

    sv.isExt = 0;

    // temporary hack
    ud.user_map = G_HaveUserMap();

#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_resetlights();
#endif

    VM_OnEvent(EVENT_SAVEGAME, g_player[myconnectindex].ps->i, myconnectindex);

    portableBackupSave(sv.path, sv.name, ud.last_stateless_volume, ud.last_stateless_level);

    // SAVE!
    sv_saveandmakesnapshot(fil, sv.name, 0, 0, 0, 0, isAutoSave);

    buildvfs_fclose(fil);

    if (!g_netServer && ud.multimode < 2)
    {
        OSD_Printf("Saved: %s\n", fn);
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

    VM_OnEvent(EVENT_POSTSAVEGAME, g_player[myconnectindex].ps->i, myconnectindex);

    return 0;

saveproblem:
    ready2send = 1;
    Net_WaitForServer();

    G_RestoreTimers();
    ototalclock = totalclock;

    return -1;
}

int32_t G_LoadPlayerMaybeMulti(savebrief_t & sv)
{
    if (g_netServer || ud.multimode > 1)
    {
        Bstrcpy(apStrings[QUOTE_RESERVED4], "Multiplayer Loading Not Yet Supported");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);

//        g_player[myconnectindex].ps->gm = MODE_GAME;
        return 127;
    }
    else
    {
        int32_t c = G_LoadPlayer(sv);
        if (c == 0)
            g_player[myconnectindex].ps->gm = MODE_GAME;
        return c;
    }
}

void G_SavePlayerMaybeMulti(savebrief_t & sv, bool isAutoSave)
{
    CONFIG_WriteSetup(2);

    if (g_netServer || ud.multimode > 1)
    {
        Bstrcpy(apStrings[QUOTE_RESERVED4], "Multiplayer Saving Not Yet Supported");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
    }
    else
    {
        G_SavePlayer(sv, isAutoSave);
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
static uint8_t *writespecdata(const dataspec_t *spec, buildvfs_FILE fil, uint8_t *dump)
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
            buildvfs_fwrite(spec->ptr, Bstrlen((const char *)spec->ptr), 1, fil);  // not null-terminated!
            continue;
        }

        void *  ptr;
        int32_t cnt;

        ds_get(spec, &ptr, &cnt);

        if (cnt < 0)
        {
            OSD_Printf("wsd: cnt=%d, f=0x%x.\n", cnt, spec->flags);
            continue;
        }

        if (!ptr || !cnt)
            continue;

        if (fil)
        {
            if ((spec->flags & DS_CMP) || ((spec->flags & DS_CNTMASK) == 0 && spec->size * cnt <= savegame_comprthres))
                buildvfs_fwrite(ptr, spec->size, cnt, fil);
            else
                dfwrite_LZ4((void *)ptr, spec->size, cnt, fil);
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
static int32_t readspecdata(const dataspec_t *spec, buildvfs_kfd fil, uint8_t **dumpvar)
{
    uint8_t *  dump = dumpvar ? *dumpvar : NULL;
    auto const sptr = spec;

    for (; spec->flags != DS_END; spec++)
    {
        if (fil == buildvfs_kfd_invalid && spec->flags & (DS_NOCHK|DS_STRING|DS_CMP))  // we're updating
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
            int const ksiz = kread(fil, cmpstrbuf, siz);

            if (ksiz != siz || Bmemcmp(spec->ptr, cmpstrbuf, siz))
            {
                OSD_Printf("rsd: spec=%s, idx=%d:\n", (char *)sptr->ptr, (int32_t)(spec-sptr));

                if (ksiz!=siz)
                    OSD_Printf("    kread returned %d, expected %d.\n", ksiz, siz);
                else
                    OSD_Printf("    sp->ptr and cmpstrbuf not identical!\n");

                return -1;
            }
            continue;
        }

        void *  ptr;
        int32_t cnt;

        ds_get(spec, &ptr, &cnt);

        if (cnt < 0)
        {
            OSD_Printf("rsd: cnt<0... wtf?\n");
            return -1;
        }

        if (!ptr || !cnt)
            continue;

        if (fil != buildvfs_kfd_invalid)
        {
            auto const mem  = (dump && (spec->flags & DS_NOCHK) == 0) ? dump : (uint8_t *)ptr;
            bool const comp = !((spec->flags & DS_CNTMASK) == 0 && spec->size * cnt <= savegame_comprthres);
            int const  siz  = comp ? cnt : cnt * spec->size;
            int const  ksiz = comp ? kdfread_LZ4(mem, spec->size, siz, fil) : kread(fil, mem, siz);

            if (ksiz != siz)
            {
                OSD_Printf("rsd: spec=%s, idx=%d, mem=%p\n", (char *)sptr->ptr, (int32_t)(spec - sptr), mem);
                OSD_Printf("     (%s): read %d, expected %d!\n",
                           ((spec->flags & DS_CNTMASK) == 0 && spec->size * cnt <= savegame_comprthres) ? "uncompressed" : "compressed", ksiz, siz);

                if (ksiz == -1)
                    OSD_Printf("     read: %s\n", strerror(errno));

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

// update dump at *dumpvar with new state and write diff to *diffvar
static void cmpspecdata(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    uint8_t * dump   = *dumpvar;
    uint8_t * diff   = *diffvar;
    int       nbytes = (getnumvar(spec) + 7) >> 3;
    int const slen   = Bstrlen((const char *)spec->ptr);

    Bmemcpy(diff, spec->ptr, slen);
    diff += slen;

    while (nbytes--)
        *(diff++) = 0;  // the bitmap of indices which elements of spec have changed go here

    int eltnum = 0;

    for (spec++; spec->flags!=DS_END; spec++)
    {
        if ((spec->flags&(DS_NOCHK|DS_STRING|DS_CMP)))
            continue;

        if (spec->flags&(DS_LOADFN|DS_SAVEFN))
        {
            if ((spec->flags&(DS_PROTECTFN))==0)
                (*(void (*)())spec->ptr)();
            continue;
        }

        void *  ptr;
        int32_t cnt;

        ds_get(spec, &ptr, &cnt);

        if (cnt < 0)
        {
            OSD_Printf("csd: cnt=%d, f=0x%x\n", cnt, spec->flags);
            continue;
        }

        uint8_t * const tmptr = diff;

        docmpsd(ptr, dump, spec->size, cnt, &diff);

        if (diff != tmptr)
            (*diffvar + slen)[eltnum>>3] |= 1<<(eltnum&7);

        dump += spec->size*cnt;
        eltnum++;
    }

    *diffvar = diff;
    *dumpvar = dump;
}

#define VALOFS(bits,p,ofs) (*(((UINT(bits) *)(p)) + (ofs)))

// apply diff to dump, not to state! state is restored from dump afterwards.
static int32_t applydiff(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    uint8_t * dump   = *dumpvar;
    uint8_t * diff   = *diffvar;
    int const nbytes = (getnumvar(spec)+7)>>3;
    int const slen   = Bstrlen((const char *)spec->ptr);

    if (Bmemcmp(diff, spec->ptr, slen))  // check STRING magic (sync check)
        return 1;

    diff += slen+nbytes;

    int eltnum = -1;
    for (spec++; spec->flags != DS_END; spec++)
    {
        if ((spec->flags & (DS_NOCHK|DS_STRING|DS_CMP|DS_LOADFN|DS_SAVEFN)))
            continue;

        int const cnt = ds_getcnt(spec);
        if (cnt < 0) return 1;

        eltnum++;
        if (((*diffvar+slen)[eltnum>>3] & pow2char[eltnum&7]) == 0)
        {
            dump += spec->size * cnt;
            continue;
        }

// ----------
#define CPSINGLEVAL(Datbits)                  \
    WVAL(Datbits, dump) = VAL(Datbits, diff); \
    diff += BYTES(Datbits);                   \
    dump += BYTES(Datbits)

        if (cnt == 1)
        {
            switch (spec->size)
            {
                case 8: CPSINGLEVAL(64); continue;
                case 4: CPSINGLEVAL(32); continue;
                case 2: CPSINGLEVAL(16); continue;
                case 1: CPSINGLEVAL(8); continue;
            }
        }

#define CPELTS(Idxbits, Datbits)                             \
    do                                                       \
    {                                                        \
        UINT(Idxbits) idx;                                   \
        goto readidx_##Idxbits##_##Datbits;                  \
        do                                                   \
        {                                                    \
            VALOFS(Datbits, dump, idx) = VAL(Datbits, diff); \
            diff += BYTES(Datbits);                          \
readidx_##Idxbits##_##Datbits:                               \
            idx = VAL(Idxbits, diff);                        \
            diff += BYTES(Idxbits);                          \
        } while ((int##Idxbits##_t)idx != -1);               \
    } while (0)

#define CPDATA(Datbits)                                                            \
    do                                                                             \
    {                                                                              \
        int const elts = tabledivide32_noinline(spec->size * cnt, BYTES(Datbits)); \
        if (elts > 65536)                                                          \
            CPELTS(32, Datbits);                                                   \
        else if (elts > 256)                                                       \
            CPELTS(16, Datbits);                                                   \
        else                                                                       \
            CPELTS(8, Datbits);                                                    \
    } while (0)

        if (spec->size == 8)
            CPDATA(64);
        else if ((spec->size & 3) == 0)
            CPDATA(32);
        else if ((spec->size & 1) == 0)
            CPDATA(16);
        else
            CPDATA(8);
        dump += spec->size * cnt;
// ----------

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
    }

    *diffvar = diff;
    *dumpvar = dump;
    return 0;
}

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

#ifdef USE_OPENGL
static void sv_prespriteextsave();
static void sv_postspriteext();
#endif
#if defined LUNATIC
// Recreate Lua state.
// XXX: It may matter a great deal when this is run from if the Lua code refers
// to C-side data at file scope. Such usage is strongly discouraged though.
static void sv_create_lua_state(void)
{
    El_CreateGameState();
    G_PostCreateGameState();
}
#endif
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

static projectile_t *savegame_projectiledata;
static uint8_t       savegame_projectiles[MAXTILES >> 3];
static int32_t       savegame_projectilecnt = 0;

#define SVARDATALEN \
    ((sizeof(g_player[0].user_name)+sizeof(g_player[0].pcolor)+sizeof(g_player[0].pteam) \
      +sizeof(g_player[0].frags)+sizeof(DukePlayer_t))*MAXPLAYERS)

static uint8_t savegame_quotedef[MAXQUOTES >> 3];
static char (*savegame_quotes)[MAXQUOTELEN];
static char (*savegame_quoteredefs)[MAXQUOTELEN];
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
    { DS_SAVEFN, (void *) &sv_preprojectilesave, 0, 1 },
    { 0, savegame_projectiles, sizeof(uint8_t), MAXTILES >> 3 },
    { DS_LOADFN, (void *) &sv_preprojectileload, 0, 1 },
    { DS_DYNAMIC|DS_CNT(savegame_projectilecnt), &savegame_projectiledata, sizeof(projectile_t), (intptr_t)&savegame_projectilecnt },
    { DS_SAVEFN, (void *) &sv_postprojectilesave, 0, 1 },
    { DS_LOADFN, (void *) &sv_postprojectileload, 0, 1 },
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
    { 0, &show2dsector[0], sizeof(uint8_t), (MAXSECTORS+7)>>3 },
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
static uint8_t *dosaveplayer2(buildvfs_FILE fil, uint8_t *mem);
static int32_t doloadplayer2(buildvfs_kfd fil, uint8_t **memptr);
static void postloadplayer(int32_t savegamep);

// SVGM snapshot system
static uint32_t svsnapsiz;
static uint8_t *svsnapshot;
static uint8_t *svinitsnap;
static uint32_t svdiffsiz;
static uint8_t *svdiff;

#include "gamedef.h"

#if !defined LUNATIC
#define SV_SKIPMASK (/*GAMEVAR_SYSTEM|*/ GAMEVAR_READONLY | GAMEVAR_PTR_MASK | /*GAMEVAR_NORESET |*/ GAMEVAR_SPECIAL)

static char svgm_vars_string [] = "blK:vars";
// setup gamevar data spec for snapshotting and diffing... gamevars must be loaded when called
static void sv_makevarspec()
{
    int vcnt = 0;

    for (int i = 0; i < g_gameVarCount; i++)
        vcnt += (aGameVars[i].flags & SV_SKIPMASK) ? 0 : 1;

    for (int i=0; i<g_gameArrayCount; i++)
        vcnt += !(aGameArrays[i].flags & (GAMEARRAY_SYSTEM|GAMEARRAY_READONLY));  // SYSTEM_GAMEARRAY

    svgm_vars = (dataspec_gv_t *)Xrealloc(svgm_vars, (vcnt + 2) * sizeof(dataspec_gv_t));

    svgm_vars[0].flags = DS_STRING;
    svgm_vars[0].ptr   = svgm_vars_string;
    svgm_vars[0].cnt   = 1;

    vcnt = 1;

    for (int i = 0; i < g_gameVarCount; i++)
    {
        if (aGameVars[i].flags & SV_SKIPMASK)
            continue;

        unsigned const per = aGameVars[i].flags & GAMEVAR_USER_MASK;

        svgm_vars[vcnt].flags = 0;
        svgm_vars[vcnt].ptr   = (per == 0) ? &aGameVars[i].global : aGameVars[i].pValues;
        svgm_vars[vcnt].size  = sizeof(intptr_t);
        svgm_vars[vcnt].cnt   = (per == 0) ? 1 : (per == GAMEVAR_PERPLAYER ? MAXPLAYERS : MAXSPRITES);

        ++vcnt;
    }

    for (int i = 0; i < g_gameArrayCount; i++)
    {
        // We must not update read-only SYSTEM_GAMEARRAY gamearrays: besides
        // being questionable by itself, sizeof(...) may be e.g. 4 whereas the
        // actual element type is int16_t (such as tilesizx[]/tilesizy[]).
        if (aGameArrays[i].flags & (GAMEARRAY_SYSTEM|GAMEARRAY_READONLY))
            continue;

        auto const pValues = aGameArrays[i].pValues;

        svgm_vars[vcnt].flags = 0;
        svgm_vars[vcnt].ptr   = pValues;

        if (aGameArrays[i].flags & GAMEARRAY_BITMAP)
        {
            svgm_vars[vcnt].size = (aGameArrays[i].size + 7) >> 3;
            svgm_vars[vcnt].cnt  = 1;  // assumed constant throughout demo, i.e. no RESIZEARRAY
        }
        else
        {
            svgm_vars[vcnt].size = pValues == NULL ? 0 : sizeof(aGameArrays[0].pValues[0]);
            svgm_vars[vcnt].cnt  = aGameArrays[i].size;  // assumed constant throughout demo, i.e. no RESIZEARRAY
        }

        ++vcnt;
    }

    svgm_vars[vcnt].flags = DS_END;
    svgm_vars[vcnt].ptr   = NULL;
    svgm_vars[vcnt].size  = 0;
    svgm_vars[vcnt].cnt   = 0;
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
int32_t sv_saveandmakesnapshot(buildvfs_FILE fil, char const *name, int8_t spot, int8_t recdiffsp, int8_t diffcompress, int8_t synccompress, bool isAutoSave)
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
    Bmemcpy(h.headerstr, "E32SAVEGAME", 11);
    h.majorver = SV_MAJOR_VER;
    h.minorver = SV_MINOR_VER;
    h.ptrsize  = sizeof(intptr_t);

    if (isAutoSave)
        h.ptrsize |= 1u << 7u;

    h.bytever      = BYTEVERSION;
    h.userbytever  = ud.userbytever;
    h.scriptcrc    = g_scriptcrc;
    h.comprthres   = savegame_comprthres;
    h.recdiffsp    = recdiffsp;
    h.diffcompress = savegame_diffcompress;
    h.synccompress = synccompress;

    h.reccnt  = 0;
    h.snapsiz = svsnapsiz;

    // the following is kinda redundant, but we save it here to be able to quickly fetch
    // it in a savegame header read

    h.numplayers = ud.multimode;
    h.volnum     = ud.volume_number;
    h.levnum     = ud.level_number;
    h.skill      = ud.player_skill;

    const uint32_t BSZ = sizeof(h.boardfn);
    EDUKE32_STATIC_ASSERT(BSZ == sizeof(currentboardfilename));
    Bstrncpy(h.boardfn, currentboardfilename, BSZ);

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

        const time_t t = time(NULL);
        struct tm *  st;

        Bstrncpyz(h.savename, "EDuke32 demo", sizeof(h.savename));
        if (t>=0 && (st = localtime(&t)))
            Bsnprintf(h.savename, sizeof(h.savename), "Demo %04d%02d%02d %s",
                      st->tm_year+1900, st->tm_mon+1, st->tm_mday, s_buildRev);
    }


    // write header
    buildvfs_fwrite(&h, sizeof(savehead_t), 1, fil);

    // for savegames, the file offset after the screenshot goes here;
    // for demos, we keep it 0 to signify that we didn't save one
    buildvfs_fwrite("\0\0\0\0", 4, 1, fil);
    if (spot >= 0 && waloff[TILE_SAVESHOT])
    {
        int32_t ofs;

        // write the screenshot compressed
        dfwrite_LZ4((char *)waloff[TILE_SAVESHOT], 320, 200, fil);

        // write the current file offset right after the header
        ofs = buildvfs_ftell(fil);
        buildvfs_fseek_abs(fil, sizeof(savehead_t));
        buildvfs_fwrite(&ofs, 4, 1, fil);
        buildvfs_fseek_abs(fil, ofs);
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
        // demo
        SV_AllocSnap(0);

        uint8_t * const p = dosaveplayer2(fil, svsnapshot);

        if (p != svsnapshot+svsnapsiz)
        {
            OSD_Printf("sv_saveandmakesnapshot: ptr-(snapshot end)=%d!\n", (int32_t)(p - (svsnapshot + svsnapsiz)));
            return 1;
        }
    }

    return 0;
}

// if file is not an EDuke32 savegame/demo, h->headerstr will be all zeros
int32_t sv_loadheader(buildvfs_kfd fil, int32_t spot, savehead_t *h)
{
    int32_t havedemo = (spot < 0);

    if (kread(fil, h, sizeof(savehead_t)) != sizeof(savehead_t))
    {
        OSD_Printf("%s %d header corrupt.\n", havedemo ? "Demo":"Savegame", havedemo ? -spot : spot);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -1;
    }

    if (Bmemcmp(h->headerstr, "E32SAVEGAME", 11)
#if 1
        && Bmemcmp(h->headerstr, "EDuke32SAVE", 11)
#endif
       )
    {
        char headerCstr[sizeof(h->headerstr) + 1];
        Bmemcpy(headerCstr, h->headerstr, sizeof(h->headerstr));
        headerCstr[sizeof(h->headerstr)] = '\0';
        OSD_Printf("%s %d header reads \"%s\", expected \"E32SAVEGAME\".\n",
                   havedemo ? "Demo":"Savegame", havedemo ? -spot : spot, headerCstr);
        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -2;
    }

    if (h->majorver != SV_MAJOR_VER || h->minorver != SV_MINOR_VER || h->bytever != BYTEVERSION || h->userbytever != ud.userbytever || (apScript != NULL && h->scriptcrc != g_scriptcrc))
    {
#ifndef DEBUGGINGAIDS
        if (havedemo)
#endif
            OSD_Printf("Incompatible savegame. Expected version %d.%d.%d.%d.%0x, found %d.%d.%d.%d.%0x\n", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                       ud.userbytever, g_scriptcrc, h->majorver, h->minorver, h->bytever, h->userbytever, h->scriptcrc);

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
            OSD_Printf("File incompatible. Expected pointer size %d, found %d\n",
                       (int32_t)sizeof(intptr_t), h->getPtrSize());

        Bmemset(h->headerstr, 0, sizeof(h->headerstr));
        return -4;
    }

    return 0;
}

int32_t sv_loadsnapshot(buildvfs_kfd fil, int32_t spot, savehead_t *h)
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


uint32_t sv_writediff(buildvfs_FILE fil)
{
    uint8_t *p = svsnapshot;
    uint8_t *d = svdiff;

    cmpspecdata(svgm_udnetw, &p, &d);
    cmpspecdata(svgm_secwsp, &p, &d);
    cmpspecdata(svgm_script, &p, &d);
    cmpspecdata(svgm_anmisc, &p, &d);
#if !defined LUNATIC
    cmpspecdata((const dataspec_t *)svgm_vars, &p, &d);
#endif

    if (p != svsnapshot+svsnapsiz)
        OSD_Printf("sv_writediff: dump+siz=%p, p=%p!\n", svsnapshot+svsnapsiz, p);

    uint32_t const diffsiz = d - svdiff;

    buildvfs_fwrite("dIfF",4,1,fil);
    buildvfs_fwrite(&diffsiz, sizeof(diffsiz), 1, fil);

    if (savegame_diffcompress)
        dfwrite_LZ4(svdiff, 1, diffsiz, fil);  // cnt and sz swapped
    else
        buildvfs_fwrite(svdiff, 1, diffsiz, fil);

    return diffsiz;
}

int32_t sv_readdiff(buildvfs_kfd fil)
{
    int32_t diffsiz;

    if (kread(fil, &diffsiz, sizeof(uint32_t)) != sizeof(uint32_t))
        return -1;

    if (savegame_diffcompress)
    {
        if (kdfread_LZ4(svdiff, 1, diffsiz, fil) != diffsiz)  // cnt and sz swapped
            return -2;
    }
    else
    {
        if (kread(fil, svdiff, diffsiz) != diffsiz)
            return -2;
    }

    uint8_t *p = svsnapshot;
    uint8_t *d = svdiff;

    if (applydiff(svgm_udnetw, &p, &d)) return -3;
    if (applydiff(svgm_secwsp, &p, &d)) return -4;
    if (applydiff(svgm_script, &p, &d)) return -5;
    if (applydiff(svgm_anmisc, &p, &d)) return -6;
#if !defined LUNATIC
    if (applydiff((const dataspec_t *)svgm_vars, &p, &d)) return -7;
#endif

    int i = 0;

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
    ud.m_level_number      = ud.level_number;
    ud.m_volume_number     = ud.volume_number;
    ud.m_player_skill      = ud.player_skill;
    ud.m_respawn_monsters  = ud.respawn_monsters;
    ud.m_respawn_items     = ud.respawn_items;
    ud.m_respawn_inventory = ud.respawn_inventory;
    ud.m_monsters_off      = ud.monsters_off;
    ud.m_coop              = ud.coop;
    ud.m_marker            = ud.marker;
    ud.m_ffire             = ud.ffire;
    ud.m_noexits           = ud.noexits;
#endif
}
//static int32_t lockclock_dummy;

#ifdef USE_OPENGL
static void sv_prespriteextsave()
{
    for (int i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
        {
            spriteext[i].mdanimtims -= mdtims;
            if (spriteext[i].mdanimtims==0)
                spriteext[i].mdanimtims++;
        }
}
static void sv_postspriteext()
{
    for (int i=0; i<MAXSPRITES; i++)
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
    Bmemset(savegame_quotedef, 0, sizeof(savegame_quotedef));
    for (int i = 0; i < MAXQUOTES; i++)
        if (apStrings[i])
        {
            savegame_quotedef[i>>3] |= 1<<(i&7);
            Bmemcpy(savegame_quotes[i], apStrings[i], MAXQUOTELEN);
        }
}
static void sv_quoteload()
{
    for (int i = 0; i < MAXQUOTES; i++)
        if (savegame_quotedef[i>>3] & pow2char[i&7])
        {
            C_AllocQuote(i);
            Bmemcpy(apStrings[i], savegame_quotes[i], MAXQUOTELEN);
        }
}

static void sv_preprojectilesave()
{
    savegame_projectilecnt = 0;
    Bmemset(savegame_projectiles, 0, sizeof(savegame_projectiles));

    for (auto & i : g_tile)
        if (i.proj)
            savegame_projectilecnt++;

    if (savegame_projectilecnt > 0)
        savegame_projectiledata = (projectile_t *) Xrealloc(savegame_projectiledata, sizeof(projectile_t) * savegame_projectilecnt);

    for (int i = 0, cnt = 0; i < MAXTILES; i++)
    {
        if (g_tile[i].proj)
        {
            savegame_projectiles[i>>3] |= 1<<(i&7);
            Bmemcpy(&savegame_projectiledata[cnt++], g_tile[i].proj, sizeof(projectile_t));
        }
    }
}

static void sv_postprojectilesave()
{
    DO_FREE_AND_NULL(savegame_projectiledata);
}

static void sv_preprojectileload()
{
    savegame_projectilecnt = 0;

    for (int i = 0; i < MAXTILES; i++)
    {
        if (savegame_projectiles[i>>3] & pow2char[i&7])
            savegame_projectilecnt++;
    }

    if (savegame_projectilecnt > 0)
        savegame_projectiledata = (projectile_t *) Xrealloc(savegame_projectiledata, sizeof(projectile_t) * savegame_projectilecnt);
}

static void sv_postprojectileload()
{
    for (int i = 0, cnt = 0; i < MAXTILES; i++)
    {
        if (savegame_projectiles[i>>3] & pow2char[i&7])
        {
            C_AllocProjectile(i);
            Bmemcpy(g_tile[i].proj, &savegame_projectiledata[cnt++], sizeof(projectile_t));
        }
    }

    DO_FREE_AND_NULL(savegame_projectiledata);
}

static void sv_prequoteredef()
{
    // "+1" needed for dfwrite which doesn't handle the src==NULL && cnt==0 case
    void *ptr = Xcalloc(g_numXStrings+1, MAXQUOTELEN);
    savegame_quoteredefs = (decltype(savegame_quoteredefs))ptr;
}
static void sv_quoteredefsave()
{
    for (int i = 0; i < g_numXStrings; i++)
        if (apXStrings[i])
            Bmemcpy(savegame_quoteredefs[i], apXStrings[i], MAXQUOTELEN);
}
static void sv_quoteredefload()
{
    for (int i = 0; i < g_numXStrings; i++)
    {
        if (!apXStrings[i])
            apXStrings[i] = (char *)Xcalloc(1,MAXQUOTELEN);
        Bmemcpy(apXStrings[i], savegame_quoteredefs[i], MAXQUOTELEN);
    }
}
static void sv_postquoteredef()
{
    Xfree(savegame_quoteredefs), savegame_quoteredefs=NULL;
}
static void sv_restsave()
{
    uint8_t *    mem = savegame_restdata;
    DukePlayer_t dummy_ps;

    Bmemset(&dummy_ps, 0, sizeof(DukePlayer_t));

#define CPDAT(ptr,sz) do { Bmemcpy(mem, ptr, sz), mem+=sz ; } while (0)
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        CPDAT(g_player[i].ps ? g_player[i].ps : &dummy_ps, sizeof(DukePlayer_t));
    }

    Bassert((savegame_restdata + SVARDATALEN) - mem == 0);
#undef CPDAT
}
static void sv_restload()
{
    uint8_t *    mem = savegame_restdata;
    DukePlayer_t dummy_ps;

#define CPDAT(ptr,sz) Bmemcpy(ptr, mem, sz), mem+=sz
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        CPDAT(g_player[i].ps ? g_player[i].ps : &dummy_ps, sizeof(DukePlayer_t));
    }
#undef CPDAT

    if (g_player[myconnectindex].ps)
        g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
}

#ifdef DEBUGGINGAIDS
# define PRINTSIZE(name) do { if (mem) OSD_Printf(name ": %d\n", (int32_t)(mem-tmem)); \
        OSD_Printf(name ": %d ms\n", timerGetTicks()-t); t=timerGetTicks(); tmem=mem; } while (0)
#else
# define PRINTSIZE(name) do { } while (0)
#endif

#ifdef LUNATIC
// <levelnum>: if we're not serializing for a mapstate, -1
//  otherwise, the linearized level number
LUNATIC_CB const char *(*El_SerializeGamevars)(int32_t *slenptr, int32_t levelnum);
#endif

static uint8_t *dosaveplayer2(buildvfs_FILE fil, uint8_t *mem)
{
#ifdef DEBUGGINGAIDS
    uint8_t *tmem = mem;
    int32_t t=timerGetTicks();
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

        buildvfs_fwrite("\0\1LunaGVAR\3\4", 12, 1, fil);
        slen_ext = B_LITTLE32(slen);
        buildvfs_fwrite(&slen_ext, sizeof(slen_ext), 1, fil);
        dfwrite_LZ4(svcode, 1, slen, fil);  // cnt and sz swapped

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

static int32_t El_ReadSaveCode(buildvfs_kfd fil)
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

        if (kdfread_LZ4(svcode, 1, slen, fil) != slen)  // cnt and sz swapped
        {
            OSD_Printf("doloadplayer2: failed reading Lunatic gamevar restoration code.\n");
            Xfree(svcode);
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
    Xfree(g_elSavecode);
    g_elSavecode = NULL;
}
#endif

static int32_t doloadplayer2(buildvfs_kfd fil, uint8_t **memptr)
{
    uint8_t *mem = memptr ? *memptr : NULL;
#ifdef DEBUGGINGAIDS
    uint8_t *tmem=mem;
    int32_t t=timerGetTicks();
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

    if (readspecdata(svgm_udnetw, buildvfs_kfd_invalid, &p)) return -2;
    if (readspecdata(svgm_secwsp, buildvfs_kfd_invalid, &p)) return -4;
    if (readspecdata(svgm_script, buildvfs_kfd_invalid, &p)) return -5;
    if (readspecdata(svgm_anmisc, buildvfs_kfd_invalid, &p)) return -6;

#if !defined LUNATIC
    if (readspecdata((const dataspec_t *)svgm_vars, buildvfs_kfd_invalid, &p)) return -8;
#endif

    if (p != pbeg+svsnapsiz)
    {
        OSD_Printf("sv_updatestate: ptr-(snapshot end)=%d\n", (int32_t)(p-(pbeg+svsnapsiz)));
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
        int32_t musicIdx = (ud.music_episode*MAXLEVELS) + ud.music_level;

        Bmemset(gotpic, 0, sizeof(gotpic));
        S_ClearSoundLocks();
        G_CacheMapData();

        if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0 && ud.music_level == USERMAPMUSICFAKELEVEL && ud.music_episode == USERMAPMUSICFAKEVOLUME)
        {
            char levname[BMAX_PATH];
            G_SetupFilenameBasedMusic(levname, boardfilename);
        }

        if (g_mapInfo[musicIdx].musicfn != NULL && (musicIdx != g_musicIndex || different_user_map))
        {
            ud.music_episode = g_musicIndex / MAXLEVELS;
            ud.music_level   = g_musicIndex % MAXLEVELS;
            S_PlayLevelMusicOrNothing(musicIdx);
        }
        else
            S_ContinueLevelMusic();

        if (ud.config.MusicToggle)
            S_PauseMusic(false);

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
#ifndef EDUKE32_STANDALONE
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

#ifdef USE_STRUCT_TRACKERS
    Bmemset(sectorchanged, 0, sizeof(sectorchanged));
    Bmemset(spritechanged, 0, sizeof(spritechanged));
    Bmemset(wallchanged, 0, sizeof(wallchanged));
#endif

#ifdef USE_OPENGL
    Polymost_prepare_loadboard();
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
}

////////// END GENERIC SAVING/LOADING SYSTEM //////////
