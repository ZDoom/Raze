//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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
#include "savegame.h"
#include "statistics.h"
#include "menu/menu.h"
#include "mapinfo.h"
#include "cmdlib.h"
#include "v_2ddrawer.h"
#include "secrets.h"
#include "sbar.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

void lava_cleararrays();
void addjaildoor(int p1, int p2, int iht, int jlt, int p3, int h);
void addminecart(int p1, int p2, int i, int iht, int p3, int childsectnum);
void addtorch(int i);
void addlightning(int i);


extern int which_palookup;

static uint8_t precachehightile[2][MAXTILES>>3];
static int32_t g_precacheCount;
int32_t g_skillSoundVoice = -1;


static inline int G_CheckExitSprite(int spriteNum) { return ((uint16_t)sprite[spriteNum].lotag == UINT16_MAX && (sprite[spriteNum].cstat & 16)); }

void G_InitRRRASkies(void)
{
    if (!RRRA)
        return;
    
    for (bssize_t i = 0; i < MAXSECTORS; i++)
    {
        if (sector[i].ceilingpicnum != TILE_LA && sector[i].ceilingpicnum != TILE_MOONSKY1 && sector[i].ceilingpicnum != TILE_BIGORBIT1)
        {
            int const picnum = sector[i].ceilingpicnum;
            if (tileWidth(picnum) == 512)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 1;
                sky->tileofs[0] = 0;
                sky->tileofs[1] = 0;
            }
            else if (tileWidth(picnum) == 1024)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 0;
                sky->tileofs[0] = 0;
            }
        }
    }
}

void prelevel_d(int g);
void prelevel_r(int g);
void e4intro(CompletionFunc completion);

void G_NewGame(int volumeNum, int levelNum, int skillNum)
{
    DukePlayer_t *const pPlayer = g_player[0].ps;

    G_HandleAsync();

    g_skillSoundVoice = -1;

    ready2send = 0;

#if 0
    if (m_recstat != 2 && ud.last_level >= 0 &&
        (g_netServer || ud.multimode > 1) && (ud.coop&GAMETYPE_SCORESHEET))
    {
        G_BonusScreen(1);
    }

    if (RR && !RRRA && ud.level_number == 6 && ud.volume_number == 0)
        G_BonusScreen(0);
#endif

    show_shareware = GAMETICSPERSEC*34;

    ud.level_number = levelNum;
    ud.volume_number = volumeNum;
    ud.player_skill = skillNum;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;
    
    int const UserMap = Menu_HaveUserMap();

    // we don't want the intro to play after the multiplayer setup screen
    if (!RR && (!g_netServer && ud.multimode < 2) && UserMap == 0 &&
        levelNum == 0 && volumeNum == 3)
    {
        e4intro([](bool) {});
    }

#ifdef EDUKE32_TOUCH_DEVICES
    pPlayer->zoom = 360;
#else
    pPlayer->zoom = 768;
#endif
    pPlayer->gm = 0;
	M_ClearMenus();

    ResetGameVars();

    //AddLog("Newgame");

    if (m_coop != 1)
    {
        for (bssize_t weaponNum = 0; weaponNum < 12/*MAX_WEAPONS*/; weaponNum++)
        {
            auto const worksLike = WW2GI ? PWEAPON(0, weaponNum, WorksLike) : weaponNum;
            if (worksLike == PISTOL_WEAPON)
            {
                pPlayer->curr_weapon = weaponNum;
                pPlayer->gotweapon.Set(weaponNum);
                pPlayer->ammo_amount[weaponNum] = min<int16_t>(max_ammo_amount[weaponNum], 48);
            }
            else if (worksLike == KNEE_WEAPON || (!RR && worksLike == HANDREMOTE_WEAPON) || (RRRA && worksLike == SLINGBLADE_WEAPON))
            {
                pPlayer->gotweapon.Set(weaponNum);
                if (RRRA)
                    pPlayer->ammo_amount[KNEE_WEAPON] = 1;
            }
        }
        pPlayer->last_weapon = -1;
    }

    display_mirror = 0;
}

void resetpspritevars(int gameMode);

static inline void clearfrags(void)
{
    for (bssize_t i = 0; i < ud.multimode; i++)
    {
        playerdata_t *const pPlayerData = &g_player[i];
        pPlayerData->ps->frag = pPlayerData->ps->fraggedself = 0;
        Bmemset(pPlayerData->frags, 0, sizeof(pPlayerData->frags));
    }
}

void G_ResetTimers(uint8_t keepgtics)
{
    totalclock = cloudtotalclock = ototalclock = lockclock = 0;
    ready2send = 1;
    levelTextTime = 85;

    if (!keepgtics)
        g_moveThingsCount = 0;

    if (camsprite >= 0)
        actor[camsprite].t_data[0] = 0;
}

int G_FindLevelByFile(const char *fileName)
{
    for (bssize_t volumeNum = 0; volumeNum < MAXVOLUMES; volumeNum++)
    {
        int const volOffset = volumeNum * MAXLEVELS;

        for (bssize_t levelNum = 0; levelNum < MAXLEVELS; levelNum++)
        {
             if (!mapList[volOffset + levelNum].fileName.CompareNoCase(fileName))
                return volOffset + levelNum;
        }
    }

    return MAXLEVELS * MAXVOLUMES;
}

static int G_TryMapHack(const char *mhkfile)
{
    int32_t failure = engineLoadMHK(mhkfile);

    if (!failure)
        Printf("Loaded map hack file \"%s\"\n", mhkfile);

    return failure;
}

static void G_LoadMapHack(char *outbuf, const char *filename)
{
    if (filename != NULL)
        Bstrcpy(outbuf, filename);

    append_ext_UNSAFE(outbuf, ".mhk");

    if (G_TryMapHack(outbuf) && usermaphacks != NULL)
    {
        usermaphack_t *pMapInfo = (usermaphack_t*)bsearch(
            &g_loadedMapHack, usermaphacks, num_usermaphacks, sizeof(usermaphack_t),
            compare_usermaphacks);

        if (pMapInfo)
            G_TryMapHack(pMapInfo->mhkfile);
    }
}

void cacheit_d();
void cacheit_r();

static int LoadTheMap(MapRecord &mi, DukePlayer_t *pPlayer, int gameMode)
{
    char levelName[BMAX_PATH];
    int16_t lbang;
    if (!VOLUMEONE && Menu_HaveUserMap())
    {
        if (engineLoadBoard(boardfilename, 0, &pPlayer->pos, &lbang, &pPlayer->cursectnum) < 0)
        {
            Printf(TEXTCOLOR_RED "Map \"%s\" not found or invalid map version!\n", boardfilename);
            return 1;
        }
        userMapRecord.name = "";
        userMapRecord.SetFileName(boardfilename);
        currentLevel = &userMapRecord;
        SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
        STAT_NewLevel(boardfilename);
        G_LoadMapHack(levelName, boardfilename);
        userMapRecord.music = G_SetupFilenameBasedMusic(boardfilename, !RR ? "dethtoll.mid" : nullptr);
    }
    else if (engineLoadBoard(mi.fileName, VOLUMEONE, &pPlayer->pos, &lbang, &pPlayer->cursectnum) < 0)
    {
        Printf(TEXTCOLOR_RED "Map \"%s\" not found or invalid map version!\n", mi.fileName.GetChars());
        return 1;
    }
    else
    {
        currentLevel = &mi;
        SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
        STAT_NewLevel(mi.fileName);
        G_LoadMapHack(levelName, mi.fileName);
    }

    if (RR && !RRRA && ud.volume_number == 1 && ud.level_number == 1)
    {
        for (bssize_t i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            g_player[0].ps->ammo_amount[i] = 0;
        g_player[0].ps->gotweapon.Clear(KNEE_WEAPON);
    }

    pPlayer->q16ang = fix16_from_int(lbang);

    g_precacheCount = 0;
    Bmemset(gotpic, 0, sizeof(gotpic));
    Bmemset(precachehightile, 0, sizeof(precachehightile));

    if (isRR()) prelevel_r(gameMode);
    else prelevel_d(gameMode);

    G_InitRRRASkies();

    if (RRRA && ud.level_number == 2 && ud.volume_number == 0)
    {
        for (bssize_t i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            g_player[0].ps->ammo_amount[i] = 0;
        g_player[0].ps->gotweapon.Clear(KNEE_WEAPON);
        g_player[0].ps->gotweapon.Set(SLINGBLADE_WEAPON);
        g_player[0].ps->ammo_amount[SLINGBLADE_WEAPON] = 1;
        g_player[0].ps->curr_weapon = SLINGBLADE_WEAPON;
    }

    allignwarpelevators();
    resetpspritevars(gameMode);

    if (isRR()) cacheit_r(); else cacheit_d();
    return 0;
}

int G_EnterLevel(int gameMode)
{
    int32_t i, mii;

//    flushpackets();
//    waitforeverybody();

    ud.respawn_monsters  = ud.m_respawn_monsters;
    ud.respawn_items     = ud.m_respawn_items;
    ud.respawn_inventory = ud.m_respawn_inventory;
    ud.monsters_off      = ud.m_monsters_off;
    ud.coop              = m_coop;
    ud.marker            = m_marker;
    ud.ffire             = m_ffire;
    ud.noexits           = m_noexits;

    if ((gameMode & MODE_DEMO) != MODE_DEMO)
        ud.recstat = m_recstat;
    if ((gameMode & MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    VM_OnEvent(EVENT_ENTERLEVEL);

    //if (g_networkMode != NET_DEDICATED_SERVER)
    {
        S_PauseSounds(false);
        FX_StopAllSounds();
        S_ClearSoundLocks();
        FX_SetReverb(0);
    }

    if (Menu_HaveUserMap())
    {
        int levelNum = G_FindLevelByFile(boardfilename);

        if (levelNum != MAXLEVELS*MAXVOLUMES)
        {
            int volumeNum = levelNum;

            levelNum &= MAXLEVELS-1;
            volumeNum = (volumeNum - levelNum) / MAXLEVELS;

            ud.level_number = m_level_number = levelNum;
            ud.volume_number = ud.m_volume_number = volumeNum;

            boardfilename[0] = 0;
        }
    }

    // Redirect the final RR level to a valid map record so that currentLevel can point to something.
    mii = (RR && g_lastLevel)? 127 : (ud.volume_number*MAXLEVELS)+ud.level_number;
    auto& mi = mapList[mii];

    if (mi.fileName.IsEmpty() && !Menu_HaveUserMap())
    {
        Printf(TEXTCOLOR_RED "Map E%dL%d not defined!\n", ud.volume_number+1, ud.level_number+1);
        return 1;
    }

    FStringf msg("%s . . .", GStrings("TXT_LOADMAP"));
    DukePlayer_t *const pPlayer = g_player[0].ps;


    /*
    G_DoLoadScreen(msg, -1);
    */
    int res = LoadTheMap(mi, pPlayer, gameMode);
    if (res != 0) return res;

    // Try this first so that it can disable the CD player if no tracks are found.
    if (RR && !(gameMode & MODE_DEMO))
        S_PlayRRMusic();

    if (ud.recstat != 2)
    {
        if (Menu_HaveUserMap())
        {
            S_PlayLevelMusic(USERMAPMUSICFAKESLOT);
        }
        else S_PlayLevelMusic(mii);
    }

    if (gameMode & (MODE_GAME|MODE_EOL))
    {
        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->gm = MODE_GAME;
        }
    }
    else if (gameMode & MODE_RESTART)
    {
        if (ud.recstat == 2)
            g_player[myconnectindex].ps->gm = MODE_DEMO;
        else g_player[myconnectindex].ps->gm = MODE_GAME;
    }

#ifndef EDUKE32_TOUCH_DEVICES
    if (VOLUMEONE && ud.level_number == 0 && ud.recstat != 2)
        FTA(QUOTE_F1HELP,g_player[myconnectindex].ps);
#endif

    for (TRAVERSE_CONNECT(i))
    {
        int pn = sector[sprite[g_player[i].ps->i].sectnum].floorpicnum;
        if (pn == TILE_HURTRAIL || pn == TILE_FLOORSLIME || pn == TILE_FLOORPLASMA)
        {
            resetweapons(i);
            resetinventory(i);

            g_player[i].ps->gotweapon.Clear(PISTOL_WEAPON);
            g_player[i].ps->ammo_amount[PISTOL_WEAPON] = 0;

            g_player[i].ps->curr_weapon  = KNEE_WEAPON;
            g_player[i].ps->kickback_pic = 0;
        }
    }

    //PREMAP.C - replace near the my's at the end of the file

    Net_NotifyNewGame();
    Net_ResetPrediction();

    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
    setpal(g_player[myconnectindex].ps);
    renderFlushPerms();

    everyothertime = 0;
    g_globalRandom = 0;

    ud.last_level = ud.level_number+1;

    clearfifo();

    for (i=numinterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];

    g_player[myconnectindex].ps->over_shoulder_on = 0;

    clearfrags();

    G_ResetTimers(0);  // Here we go

    //Bsprintf(g_szBuf,"G_EnterLevel L=%d V=%d",ud.level_number, ud.volume_number);
    //AddLog(g_szBuf);
    // variables are set by pointer...


    if (G_HaveUserMap())
    {
        Printf(TEXTCOLOR_GOLD "%s: %s\n", GStrings("TXT_USERMAP"), boardfilename);
    }
    else
    {
        Printf(TEXTCOLOR_GOLD "E%dL%d: %s\n", ud.volume_number+1, ud.level_number+1,
                   mapList[mii].DisplayName());
    }

    restorepalette = -1;

    videoClearViewableArea(0L);
    displayrooms(myconnectindex,65536);
    G_DisplayRest(65536);

    Net_WaitForEverybody();
    return 0;
}

void setmapfog(int fogtype)
{
    GLInterface.SetMapFog(fogtype != 0);
}

END_DUKE_NS
