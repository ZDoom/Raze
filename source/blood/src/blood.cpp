//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "build.h"
#include "mmulti.h"
#include "compat.h"
#include "renderlayer.h"
#include "fx_man.h"
#include "common.h"
#include "common_game.h"
#include "gamedefs.h"

#include "asound.h"
#include "db.h"
#include "blood.h"
#include "choke.h"
#include "config.h"
#include "controls.h"
#include "credits.h"
#include "demo.h"
#include "dude.h"
#include "endgame.h"
#include "eventq.h"
#include "fire.h"
#include "fx.h"
#include "getopt.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "menu.h"
#include "mirrors.h"
#include "music.h"
#include "network.h"
#include "osdcmds.h"
#include "replace.h"
#include "resource.h"
#include "qheap.h"
#include "screen.h"
#include "sectorfx.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "tile.h"
#include "trig.h"
#include "triggers.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"

#ifdef _WIN32
# include <shellapi.h>
# define UPDATEINTERVAL 604800 // 1w
# include "winbits.h"
#else
# ifndef GEKKO
#  include <sys/ioctl.h>
# endif
#endif /* _WIN32 */

const char* AppProperName = APPNAME;
const char* AppTechnicalName = APPBASENAME;

char SetupFilename[BMAX_PATH] = SETUPFILENAME;
int32_t gNoSetup = 0, gCommandSetup = 0;

INPUT_MODE gInputMode;

unsigned int nMaxAlloc = 0x4000000;

bool bCustomName = false;
char bAddUserMap = false;
bool bNoDemo = false;
bool bQuickStart = true;
bool bNoAutoLoad = false;

int gMusicPrevLoadedEpisode = -1;
int gMusicPrevLoadedLevel = -1;

char gUserMapFilename[BMAX_PATH];
char gPName[MAXPLAYERNAME];

short BloodVersion = 0x115;

int gNetPlayers;

char *pUserTiles = NULL;
char *pUserSoundRFF = NULL;
char *pUserRFF = NULL;

int gChokeCounter = 0;

double g_gameUpdateTime, g_gameUpdateAndDrawTime;
double g_gameUpdateAvgTime = 0.001;

int gSaveGameNum;
bool gQuitGame;
int gQuitRequest;
bool gPaused;
bool gSaveGameActive;
int gCacheMiss;

enum gametokens
{
    T_INCLUDE = 0,
    T_INTERFACE = 0,
    T_LOADGRP = 1,
    T_MODE = 1,
    T_CACHESIZE = 2,
    T_ALLOW = 2,
    T_NOAUTOLOAD,
    T_INCLUDEDEFAULT,
    T_MUSIC,
    T_SOUND,
    T_FILE,
    //T_CUTSCENE,
    //T_ANIMSOUNDS,
    //T_NOFLOORPALRANGE,
    T_ID,
    T_MINPITCH,
    T_MAXPITCH,
    T_PRIORITY,
    T_TYPE,
    T_DISTANCE,
    T_VOLUME,
    T_DELAY,
    T_RENAMEFILE,
    T_GLOBALGAMEFLAGS,
    T_ASPECT,
    T_FORCEFILTER,
    T_FORCENOFILTER,
    T_TEXTUREFILTER,
    T_RFFDEFINEID,
};

int blood_globalflags;

void app_crashhandler(void)
{
    // NUKE-TODO:
}

void G_Polymer_UnInit(void)
{
    // NUKE-TODO:
}

void M32RunScript(const char *s)
{
    UNREFERENCED_PARAMETER(s);
}

void ShutDown(void)
{
    if (!in3dmode())
        return;
    CONFIG_WriteSetup(0);
    netDeinitialize();
    sndTerm();
    sfxTerm();
    scrUnInit();
    CONTROL_Shutdown();
    KB_Shutdown();
    OSD_Cleanup();
    // PORT_TODO: Check argument
    if (syncstate)
        printf("A packet was lost! (syncstate)\n");
    for (int i = 0; i < 10; i++)
    {
        if (gSaveGamePic[i])
            Resource::Free(gSaveGamePic[i]);
    }
    DO_FREE_AND_NULL(pUserTiles);
    DO_FREE_AND_NULL(pUserSoundRFF);
    DO_FREE_AND_NULL(pUserRFF);
}

void QuitGame(void)
{
    ShutDown();
    exit(0);
}

int nPrecacheCount;

void PrecacheDude(spritetype *pSprite)
{
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    seqPrecacheId(pDudeInfo->seqStartID);
    seqPrecacheId(pDudeInfo->seqStartID+5);
    seqPrecacheId(pDudeInfo->seqStartID+1);
    seqPrecacheId(pDudeInfo->seqStartID+2);
    switch (pSprite->type)
    {
    case 201:
    case 202:
    case 247:
    case 248:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        seqPrecacheId(pDudeInfo->seqStartID+13);
        seqPrecacheId(pDudeInfo->seqStartID+14);
        seqPrecacheId(pDudeInfo->seqStartID+15);
        break;
    case 204:
    case 217:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        seqPrecacheId(pDudeInfo->seqStartID+10);
        seqPrecacheId(pDudeInfo->seqStartID+11);
        break;
    case 208:
    case 209:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+6);
        fallthrough__;
    case 206:
    case 207:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        break;
    case 210:
    case 211:
    case 213:
    case 214:
    case 215:
    case 216:
    case 229:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        break;
    case 227:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        fallthrough__;
    case 212:
    case 218:
    case 219:
    case 220:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        break;
    case 249:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        break;
    case 205:
        seqPrecacheId(pDudeInfo->seqStartID+12);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        fallthrough__;
    case 244:
        seqPrecacheId(pDudeInfo->seqStartID+10);
        fallthrough__;
    case 203:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+11);
        seqPrecacheId(pDudeInfo->seqStartID+13);
        seqPrecacheId(pDudeInfo->seqStartID+14);
        break;
    }
}

void PrecacheThing(spritetype *pSprite)
{
    switch (pSprite->type)
    {
    case 406:
    case 407:
        seqPrecacheId(12);
        break;
    case 410:
        seqPrecacheId(15);
        break;
    case 411:
        seqPrecacheId(21);
        break;
    case 412:
        seqPrecacheId(25);
        seqPrecacheId(26);
        break;
    case 413:
        seqPrecacheId(38);
        seqPrecacheId(40);
        seqPrecacheId(28);
        break;
    case 416:
        break;
    default:
        tilePreloadTile(pSprite->picnum);
        break;
    }
    seqPrecacheId(3);
    seqPrecacheId(4);
    seqPrecacheId(5);
    seqPrecacheId(9);
}

void PreloadTiles(void)
{
    int skyTile = -1;
    memset(gotpic,0,sizeof(gotpic));
    for (int i = 0; i < numsectors; i++)
    {
        tilePrecacheTile(sector[i].floorpicnum, 0);
        tilePrecacheTile(sector[i].ceilingpicnum, 0);
        if ((sector[i].ceilingstat&1) != 0 && skyTile == -1)
            skyTile = sector[i].ceilingpicnum;
    }
    for (int i = 0; i < numwalls; i++)
    {
        tilePrecacheTile(wall[i].picnum, 0);
        if (wall[i].overpicnum >= 0)
            tilePrecacheTile(wall[i].overpicnum, 0);
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            spritetype *pSprite = &sprite[i];
            switch (pSprite->statnum)
            {
            case 6:
                PrecacheDude(pSprite);
                break;
            case 4:
                PrecacheThing(pSprite);
                break;
            default:
                tilePrecacheTile(pSprite->picnum);
                break;
            }
        }
    }
    if (numplayers > 1)
    {
        seqPrecacheId(dudeInfo[31].seqStartID+6);
        seqPrecacheId(dudeInfo[31].seqStartID+7);
        seqPrecacheId(dudeInfo[31].seqStartID+8);
        seqPrecacheId(dudeInfo[31].seqStartID+9);
        seqPrecacheId(dudeInfo[31].seqStartID+10);
        seqPrecacheId(dudeInfo[31].seqStartID+14);
        seqPrecacheId(dudeInfo[31].seqStartID+15);
        seqPrecacheId(dudeInfo[31].seqStartID+12);
        seqPrecacheId(dudeInfo[31].seqStartID+16);
        seqPrecacheId(dudeInfo[31].seqStartID+17);
        seqPrecacheId(dudeInfo[31].seqStartID+18);
    }
    if (skyTile > -1 && skyTile < kMaxTiles)
    {
        for (int i = 1; i < gSkyCount; i++)
            tilePrecacheTile(skyTile+i, 0);
    }
    G_HandleAsync();
}

char precachehightile[2][(MAXTILES+7)>>3];
#ifdef USE_OPENGL
void PrecacheExtraTextureMaps(int nTile)
{
    // PRECACHE
    if (useprecache && bpp > 8)
    {
        for (int type = 0; type < 2 && !KB_KeyPressed(sc_Space); type++)
        {
            if (TestBitString(precachehightile[type], nTile))
            {
                for (int k = 0; k < MAXPALOOKUPS - RESERVEDPALS && !KB_KeyPressed(sc_Space); k++)
                {
                    // this is the CROSSHAIR_PAL, see screens.cpp
                    if (k == MAXPALOOKUPS - RESERVEDPALS - 1)
                        break;
#ifdef POLYMER
                    if (videoGetRenderMode() != REND_POLYMER || !polymer_havehighpalookup(0, k))
#endif
                        polymost_precache(nTile, k, type);
                }

#ifdef USE_GLEXT
                if (r_detailmapping)
                    polymost_precache(nTile, DETAILPAL, type);

                if (r_glowmapping)
                    polymost_precache(nTile, GLOWPAL, type);
#endif
#ifdef POLYMER
                if (videoGetRenderMode() == REND_POLYMER)
                {
                    if (pr_specularmapping)
                        polymost_precache(nTile, SPECULARPAL, type);

                    if (pr_normalmapping)
                        polymost_precache(nTile, NORMALPAL, type);
                }
#endif
            }
        }
    }
}
#endif

void PreloadCache(void)
{
    char tempbuf[128];
    if (gDemo.at1)
        return;
    if (MusicRestartsOnLoadToggle)
        sndTryPlaySpecialMusic(MUS_LOADING);
    gSoundRes.PrecacheSounds();
    PreloadTiles();
    int clock = totalclock.Ticks();
    int cnt = 0;
    int percentDisplayed = -1;

    for (int i=0; i<kMaxTiles && !KB_KeyPressed(sc_Space); i++)
    {
        if (TestBitString(gotpic, i))
        {
            if (waloff[i] == 0)
                tileLoad((int16_t)i);

#ifdef USE_OPENGL
            PrecacheExtraTextureMaps(i);
#endif

            MUSIC_Update();

            if ((++cnt & 7) == 0)
                G_HandleAsync();

            if (videoGetRenderMode() != REND_CLASSIC && totalclock - clock > (kTicRate>>2))
            {
                int const percentComplete = min(100, tabledivide32_noinline(100 * cnt, nPrecacheCount));

                // this just prevents the loading screen percentage bar from making large jumps
                while (percentDisplayed < percentComplete)
                {
                    Bsprintf(tempbuf, "Loaded %d%% (%d/%d textures)\n", percentDisplayed, cnt, nPrecacheCount);
                    viewLoadingScreenUpdate(tempbuf, percentDisplayed);
                    timerUpdate();

                    if (totalclock - clock >= 1)
                    {
                        clock = totalclock.Ticks();
                        percentDisplayed++;
                    }
                }

                clock = totalclock.Ticks();
            }
        }
    }
    memset(gotpic,0,sizeof(gotpic));
}

void EndLevel(void)
{
    gViewPos = VIEWPOS_0;
    gGameMessageMgr.Clear();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
}

PLAYER gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];

vec3_t startpos;
int16_t startang, startsectnum;

void StartLevel(GAMEOPTIONS *gameOptions)
{
    EndLevel();
    gStartNewGame = 0;
    ready2send = 0;
    gMusicPrevLoadedEpisode = gGameOptions.nEpisode;
    gMusicPrevLoadedLevel = gGameOptions.nLevel;
    if (gDemo.at0 && gGameStarted)
        gDemo.Close();
    netWaitForEveryone(0);
    if (gGameOptions.nGameType == 0)
    {
        if (!(gGameOptions.uGameFlags&1))
            levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
        if (gEpisodeInfo[gGameOptions.nEpisode].cutALevel == gGameOptions.nLevel
            && gEpisodeInfo[gGameOptions.nEpisode].at8f08)
            gGameOptions.uGameFlags |= 4;
        if ((gGameOptions.uGameFlags&4) && gDemo.at1 == 0)
            levelPlayIntroScene(gGameOptions.nEpisode);
    }
    else if (gGameOptions.nGameType > 0 && !(gGameOptions.uGameFlags&1))
    {
        gGameOptions.nEpisode = gPacketStartGame.episodeId;
        gGameOptions.nLevel = gPacketStartGame.levelId;
        gGameOptions.nGameType = gPacketStartGame.gameType;
        gGameOptions.nDifficulty = gPacketStartGame.difficulty;
        gGameOptions.nMonsterSettings = gPacketStartGame.monsterSettings;
        gGameOptions.nWeaponSettings = gPacketStartGame.weaponSettings;
        gGameOptions.nItemSettings = gPacketStartGame.itemSettings;
        gGameOptions.nRespawnSettings = gPacketStartGame.respawnSettings;
        if (gPacketStartGame.userMap)
            levelAddUserMap(gPacketStartGame.userMapName);
        else
            levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    }
    if (gameOptions->uGameFlags&1)
    {
        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            memcpy(&gPlayerTemp[i],&gPlayer[i],sizeof(PLAYER));
            gHealthTemp[i] = xsprite[gPlayer[i].pSprite->extra].health;
        }
    }
    bVanilla = gDemo.at1 && gDemo.m_bLegacy;
    //blooddemohack = 1;//bVanilla;
    memset(xsprite,0,sizeof(xsprite));
    memset(sprite,0,kMaxSprites*sizeof(spritetype));
    drawLoadingScreen();
    dbLoadMap(gameOptions->zLevelName,(int*)&startpos.x,(int*)&startpos.y,(int*)&startpos.z,&startang,&startsectnum,(unsigned int*)&gameOptions->uMapCRC);
    wsrand(gameOptions->uMapCRC);
    gKillMgr.Clear();
    gSecretMgr.Clear();
    gLevelTime = 0;
    automapping = 1;
    for (int i = 0; i < kMaxSprites; i++)
    {
        spritetype *pSprite = &sprite[i];
        if (pSprite->statnum < kMaxStatus && pSprite->extra > 0)
        {
            XSPRITE *pXSprite = &xsprite[pSprite->extra];
            if ((pXSprite->lSkill & (1 << gameOptions->nDifficulty)) || (pXSprite->lS && gameOptions->nGameType == 0)
                || (pXSprite->lB && gameOptions->nGameType == 2) || (pXSprite->lT && gameOptions->nGameType == 3)
                || (pXSprite->lC && gameOptions->nGameType == 1)) {
                
                DeleteSprite(i);
                continue;
            }

            if (sprite[i].lotag == kGDXDudeTargetChanger)
                InsertSpriteStat(i, kStatGDXDudeTargetChanger);
        }
    }
    scrLoadPLUs();
    startpos.z = getflorzofslope(startsectnum,startpos.x,startpos.y);
    for (int i = 0; i < kMaxPlayers; i++)
    {
        gStartZone[i].x = startpos.x;
        gStartZone[i].y = startpos.y;
        gStartZone[i].z = startpos.z;
        gStartZone[i].sectnum = startsectnum;
        gStartZone[i].ang = startang;

        // By NoOne: Create spawn zones for players in teams mode.
        if (i <= kMaxPlayers / 2) {
            gStartZoneTeam1[i].x = startpos.x;
            gStartZoneTeam1[i].y = startpos.y;
            gStartZoneTeam1[i].z = startpos.z;
            gStartZoneTeam1[i].sectnum = startsectnum;
            gStartZoneTeam1[i].ang = startang;

            gStartZoneTeam2[i].x = startpos.x;
            gStartZoneTeam2[i].y = startpos.y;
            gStartZoneTeam2[i].z = startpos.z;
            gStartZoneTeam2[i].sectnum = startsectnum;
            gStartZoneTeam2[i].ang = startang;
        }
    }
    InitSectorFX();
    warpInit();
    actInit();
    evInit();
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (!(gameOptions->uGameFlags&1))
        {
            if (numplayers == 1)
            {
                gProfile[i].skill = gSkill;
                gProfile[i].nAutoAim = gAutoAim;
                gProfile[i].nWeaponSwitch = gWeaponSwitch;
            }
            playerInit(i,0);
        }
        playerStart(i);
    }
    if (gameOptions->uGameFlags&1)
    {
        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            PLAYER *pPlayer = &gPlayer[i];
            pPlayer->pXSprite->health &= 0xf000;
            pPlayer->pXSprite->health |= gHealthTemp[i];
            pPlayer->at26 = gPlayerTemp[i].at26;
            pPlayer->atbd = gPlayerTemp[i].atbd;
            pPlayer->atc3 = gPlayerTemp[i].atc3;
            pPlayer->atc7 = gPlayerTemp[i].atc7;
            pPlayer->at2a = gPlayerTemp[i].at2a;
            pPlayer->at1b1 = gPlayerTemp[i].at1b1;
            pPlayer->atbf = gPlayerTemp[i].atbf;
            pPlayer->atbe = gPlayerTemp[i].atbe;
        }
    }
    gameOptions->uGameFlags &= ~3;
    scrSetDac();
    PreloadCache();
    InitMirrors();
    gFrameClock = 0;
    trInit();
    if (!bVanilla && !gMe->packInfo[1].at0) // if diving suit is not active, turn off reverb sound effect
        sfxSetReverb(0);
    ambInit();
    sub_79760();
    gCacheMiss = 0;
    gFrame = 0;
    gChokeCounter = 0;
    if (!gDemo.at1)
        gGameMenuMgr.Deactivate();
    levelTryPlayMusicOrNothing(gGameOptions.nEpisode, gGameOptions.nLevel);
    // viewSetMessage("");
    viewSetErrorMessage("");
    viewResizeView(gViewSize);
    if (gGameOptions.nGameType == 3)
        gGameMessageMgr.SetCoordinates(gViewX0S+1,gViewY0S+15);
    netWaitForEveryone(0);
    gGameClock = 0;
    gPaused = 0;
    gGameStarted = 1;
    ready2send = 1;
}

void StartNetworkLevel(void)
{
    if (gDemo.at0)
        gDemo.Close();
    if (!(gGameOptions.uGameFlags&1))
    {
        gGameOptions.nEpisode = gPacketStartGame.episodeId;
        gGameOptions.nLevel = gPacketStartGame.levelId;
        gGameOptions.nGameType = gPacketStartGame.gameType;
        gGameOptions.nDifficulty = gPacketStartGame.difficulty;
        gGameOptions.nMonsterSettings = gPacketStartGame.monsterSettings;
        gGameOptions.nWeaponSettings = gPacketStartGame.weaponSettings;
        gGameOptions.nItemSettings = gPacketStartGame.itemSettings;
        gGameOptions.nRespawnSettings = gPacketStartGame.respawnSettings;
        if (gPacketStartGame.userMap)
            levelAddUserMap(gPacketStartGame.userMapName);
        else
            levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    }
    StartLevel(&gGameOptions);
}

void LocalKeys(void)
{
    char alt = keystatus[sc_LeftAlt] | keystatus[sc_RightAlt];
    char ctrl = keystatus[sc_LeftControl] | keystatus[sc_RightControl];
    char shift = keystatus[sc_LeftShift] | keystatus[sc_RightShift];
    if (BUTTON(gamefunc_See_Chase_View) && !alt && !shift)
    {
        CONTROL_ClearButton(gamefunc_See_Chase_View);
        if (gViewPos > VIEWPOS_0)
            gViewPos = VIEWPOS_0;
        else
            gViewPos = VIEWPOS_1;
    }
    if (BUTTON(gamefunc_See_Coop_View))
    {
        CONTROL_ClearButton(gamefunc_See_Coop_View);
        if (gGameOptions.nGameType == 1)
        {
            gViewIndex = connectpoint2[gViewIndex];
            if (gViewIndex == -1)
                gViewIndex = connecthead;
            gView = &gPlayer[gViewIndex];
        }
        else if (gGameOptions.nGameType == 3)
        {
            int oldViewIndex = gViewIndex;
            do
            {
                gViewIndex = connectpoint2[gViewIndex];
                if (gViewIndex == -1)
                    gViewIndex = connecthead;
                if (oldViewIndex == gViewIndex || gMe->at2ea == gPlayer[gViewIndex].at2ea)
                    break;
            } while (oldViewIndex != gViewIndex);
            gView = &gPlayer[gViewIndex];
        }
    }
    char key;
    if ((key = keyGetScan()) != 0)
    {
        if ((alt || shift) && gGameOptions.nGameType > 0 && key >= 0x3b && key <= 0x44)
        {
            char fk = key - 0x3b;
            if (alt)
            {
                netBroadcastTaunt(myconnectindex, fk);
            }
            else
            {
                gPlayerMsg.Set(CommbatMacro[fk]);
                gPlayerMsg.Send();
            }
            keyFlushScans();
            keystatus[key] = 0;
            CONTROL_ClearButton(41);
            return;
        }
        switch (key)
        {
        case 0x53:
        case 0xd3:
            if (ctrl && alt)
            {
                gQuitGame = 1;
                return;
            }
            break;
        case 0x01:
            keyFlushScans();
            if (gGameStarted && gPlayer[myconnectindex].pXSprite->health != 0)
            {
                if (!gGameMenuMgr.m_bActive)
                    gGameMenuMgr.Push(&menuMainWithSave,-1);
            }
            else
            {
                if (!gGameMenuMgr.m_bActive)
                    gGameMenuMgr.Push(&menuMain,-1);
            }
            return;
        case 0x3b:
            keyFlushScans();
            if (gGameOptions.nGameType == 0)
                gGameMenuMgr.Push(&menuOrder,-1);
            break;
        case 0x3c:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive && gGameOptions.nGameType == 0)
                gGameMenuMgr.Push(&menuSaveGame,-1);
            break;
        case 0x3d:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive && gGameOptions.nGameType == 0)
                gGameMenuMgr.Push(&menuLoadGame,-1);
            break;
        case 0x3e:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive)
                gGameMenuMgr.Push(&menuOptionsSound,-1);
            return;
        case 0x3f:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive)
                gGameMenuMgr.Push(&menuOptions,-1);
            return;
        case 0x40:
            keyFlushScans();
            if (gGameStarted && !gGameMenuMgr.m_bActive && gPlayer[myconnectindex].pXSprite->health != 0)
            {
                if (gQuickSaveSlot != -1)
                {
                    QuickSaveGame();
                    return;
                }
                gGameMenuMgr.Push(&menuSaveGame,-1);
            }
            break;
        case 0x42:
            keyFlushScans();
            gGameMenuMgr.Push(&menuOptions,-1);
            break;
        case 0x43:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive)
            {
                if (gQuickLoadSlot != -1)
                {
                    QuickLoadGame();
                    return;
                }
                if (gQuickLoadSlot == -1 && gQuickSaveSlot != -1)
                {
                    gQuickLoadSlot = gQuickSaveSlot;
                    QuickLoadGame();
                    return;
                }
                gGameMenuMgr.Push(&menuLoadGame,-1);
            }
            break;
        case 0x44:
            keyFlushScans();
            if (!gGameMenuMgr.m_bActive)
                gGameMenuMgr.Push(&menuQuit,-1);
            break;
        case 0x57:
            break;
        case 0x58:
            videoCaptureScreen("blud0000.tga", 0);
            break;
        }
    }
}

bool gRestartGame = false;

void ProcessFrame(void)
{
    char buffer[128];
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        gPlayer[i].atc.buttonFlags = gFifoInput[gNetFifoTail&255][i].buttonFlags;
        gPlayer[i].atc.keyFlags.word |= gFifoInput[gNetFifoTail&255][i].keyFlags.word;
        gPlayer[i].atc.useFlags.byte |= gFifoInput[gNetFifoTail&255][i].useFlags.byte;
        if (gFifoInput[gNetFifoTail&255][i].newWeapon)
            gPlayer[i].atc.newWeapon = gFifoInput[gNetFifoTail&255][i].newWeapon;
        gPlayer[i].atc.forward = gFifoInput[gNetFifoTail&255][i].forward;
        gPlayer[i].atc.q16turn = gFifoInput[gNetFifoTail&255][i].q16turn;
        gPlayer[i].atc.strafe = gFifoInput[gNetFifoTail&255][i].strafe;
        gPlayer[i].atc.q16mlook = gFifoInput[gNetFifoTail&255][i].q16mlook;
    }
    gNetFifoTail++;
    if (!(gFrame&((gSyncRate<<3)-1)))
    {
        CalcGameChecksum();
        memcpy(gCheckFifo[gCheckHead[myconnectindex]&255][myconnectindex], gChecksum, sizeof(gChecksum));
        gCheckHead[myconnectindex]++;
    }
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (gPlayer[i].atc.keyFlags.quit)
        {
            gPlayer[i].atc.keyFlags.quit = 0;
            netBroadcastPlayerLogoff(i);
            if (i == myconnectindex)
            {
                // netBroadcastMyLogoff(gQuitRequest == 2);
                gQuitGame = true;
                gRestartGame = gQuitRequest == 2;
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
        }
        if (gPlayer[i].atc.keyFlags.restart)
        {
            gPlayer[i].atc.keyFlags.restart = 0;
            levelRestart();
            return;
        }
        if (gPlayer[i].atc.keyFlags.pause)
        {
            gPlayer[i].atc.keyFlags.pause = 0;
            gPaused = !gPaused;
            if (gPaused && gGameOptions.nGameType > 0 && numplayers > 1)
            {
                sprintf(buffer,"%s paused the game",gProfile[i].name);
                viewSetMessage(buffer);
            }
        }
    }
    viewClearInterpolations();
    if (!gDemo.at1)
    {
        if (gPaused || gEndGameMgr.at0 || (gGameOptions.nGameType == 0 && gGameMenuMgr.m_bActive))
            return;
        if (gDemo.at0)
            gDemo.Write(gFifoInput[(gNetFifoTail-1)&255]);
    }
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        viewBackupView(i);
        playerProcess(&gPlayer[i]);
    }
    trProcessBusy();
    evProcess(gFrameClock);
    seqProcess(4);
    DoSectorPanning();
    actProcessSprites();
    actPostProcess();
    viewCorrectPrediction();
    sndProcess();
    ambProcess();
    viewUpdateDelirium();
    viewUpdateShake();
    sfxUpdate3DSounds();
    if (gMe->at376 == 1)
    {
#define CHOKERATE 8
#define TICRATE 30
        gChokeCounter += CHOKERATE;
        while (gChokeCounter >= TICRATE)
        {
            gChoke.at1c(gMe);
            gChokeCounter -= TICRATE;
        }
    }
    gLevelTime++;
    gFrame++;
    gFrameClock += 4;
    if ((gGameOptions.uGameFlags&1) != 0 && !gStartNewGame)
    {
        ready2send = 0;
        if (gNetPlayers > 1 && gNetMode == NETWORK_SERVER && gPacketMode == PACKETMODE_1 && myconnectindex == connecthead)
        {
            while (gNetFifoMasterTail < gNetFifoTail)
            {
                G_HandleAsync();
                netMasterUpdate();
            }
        }
        if (gDemo.at0)
            gDemo.Close();
        sndFadeSong(4000);
        seqKillAll();
        if (gGameOptions.uGameFlags&2)
        {
            if (gGameOptions.nGameType == 0)
            {
                if (gGameOptions.uGameFlags&8)
                    levelPlayEndScene(gGameOptions.nEpisode);
                gGameMenuMgr.Deactivate();
                gGameMenuMgr.Push(&menuCredits,-1);
            }
            gGameOptions.uGameFlags &= ~3;
            gRestartGame = 1;
            gQuitGame = 1;
        }
        else
        {
            gEndGameMgr.Setup();
            viewResizeView(gViewSize);
        }
    }
}

SWITCH switches[] = {
    { "?", 0, 0 },
    { "help", 0, 0 },
    { "broadcast", 1, 0 },
    { "map", 2, 1 },
    { "masterslave", 3, 0 },
    //{ "net", 4, 1 },
    { "nodudes", 5, 1 },
    { "playback", 6, 1 },
    { "record", 7, 1 },
    { "robust", 8, 0 },
    { "setupfile", 9, 1 },
    { "skill", 10, 1 },
    //{ "nocd", 11, 0 },
    //{ "8250", 12, 0 },
    { "ini", 13, 1 },
    { "noaim", 14, 0 },
    { "f", 15, 1 },
    { "control", 16, 1 },
    { "vector", 17, 1 },
    { "quick", 18, 0 },
    //{ "getopt", 19, 1 },
    //{ "auto", 20, 1 },
    { "pname", 21, 1 },
    { "noresend", 22, 0 },
    { "silentaim", 23, 0 },
    { "nodemo", 25, 0 },
    { "art", 26, 1 },
    { "snd", 27, 1 },
    { "rff", 28, 1 },
    { "maxalloc", 29, 1 },
    { "server", 30, 1 },
    { "client", 31, 1 },
    { "noautoload", 32, 0 },
    { "usecwd", 33, 0 },
    { "cachesize", 34, 1 },
    { "g", 35, 1 },
    { "grp", 35, 1 },
    { "game_dir", 36, 1 },
    { "cfg", 9, 1 },
    { "setup", 37, 0 },
    { "nosetup", 38, 0 },
    { "port", 39, 1 },
    { "h", 40, 1 },
    { "mh", 41, 1 },
    { "j", 42, 1 },
    { NULL, 0, 0 }
};

void PrintHelp(void)
{
    char tempbuf[128];
    static char const s[] = "Usage: " APPBASENAME " [files] [options]\n"
        "Example: " APPBASENAME " -usecwd -cfg myconfig.cfg -map nukeland.map\n\n"
        "Files can be of type [grp|zip|map|def]\n"
        "\n"
        "-art [file.art]\tSpecify an art base file name\n"
        "-cachesize #\tSet cache size in kB\n"
        "-cfg [file.cfg]\tUse an alternate configuration file\n"
        "-client [host]\tConnect to a multiplayer game\n"
        "-game_dir [dir]\tSpecify game data directory\n"
        "-g [file.grp]\tLoad additional game data\n"
        "-h [file.def]\tLoad an alternate definitions file\n"
        "-ini [file.ini]\tSpecify an INI file name (default is blood.ini)\n"
        "-j [dir]\t\tAdd a directory to " APPNAME "'s search list\n"
        "-map [file.map]\tLoad an external map file\n"
        "-mh [file.def]\tInclude an additional definitions module\n"
        "-noautoload\tDisable loading from autoload directory\n"
        "-nodemo\t\tNo Demos\n"
        "-nodudes\tNo monsters\n"
        "-playback\tPlay back a demo\n"
        "-pname\t\tOverride player name setting from config file\n"
        "-record\t\tRecord demo\n"
        "-rff\t\tSpecify an RFF file for Blood game resources\n"
        "-server [players]\tStart a multiplayer server\n"
#ifdef STARTUP_SETUP_WINDOW
        "-setup/nosetup\tEnable or disable startup window\n"
#endif
        "-skill\t\tSet player handicap; Range:0..4; Default:2; (NOT difficulty level.)\n"
        "-snd\t\tSpecify an RFF Sound file name\n"
        "-usecwd\t\tRead data and configuration from current directory\n"
        ;
#ifdef WM_MSGBOX_WINDOW
    Bsnprintf(tempbuf, sizeof(tempbuf), APPNAME " %s", s_buildRev);
    wm_msgbox(tempbuf, s);
#else
    initprintf("%s\n", s);
#endif
#if 0
    puts("Blood Command-line Options:");
    // NUKE-TODO:
    puts("-?            This help");
    //puts("-8250         Enforce obsolete UART I/O");
    //puts("-auto         Automatic Network start. Implies -quick");
    //puts("-getopt       Use network game options from file.  Implies -auto");
    puts("-broadcast    Set network to broadcast packet mode");
    puts("-masterslave  Set network to master/slave packet mode");
    //puts("-net          Net mode game");
    //puts("-noaim        Disable auto-aiming");
    //puts("-nocd         Disable CD audio");
    puts("-nodudes      No monsters");
    puts("-nodemo       No Demos");
    puts("-robust       Robust network sync checking");
    puts("-skill        Set player handicap; Range:0..4; Default:2; (NOT difficulty level.)");
    puts("-quick        Skip Intro screens and get right to the game");
    puts("-pname        Override player name setting from config file");
    puts("-map          Specify a user map");
    puts("-playback     Play back a demo");
    puts("-record       Record a demo");
    puts("-art          Specify an art base file name");
    puts("-snd          Specify an RFF Sound file name");
    puts("-RFF          Specify an RFF file for Blood game resources");
    puts("-ini          Specify an INI file name (default is blood.ini)");
#endif
    exit(0);
}

void ParseOptions(void)
{
    int option;
    while ((option = GetOptions(switches)) != -1)
    {
        switch (option)
        {
        case -3:
            ThrowError("Invalid argument: %s", OptFull);
            fallthrough__;
        case 29:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            nMaxAlloc = atoi(OptArgv[0]);
            if (!nMaxAlloc)
                nMaxAlloc = 0x2000000;
            break;
        case 0:
            PrintHelp();
            break;
        //case 19:
        //    byte_148eec = 1;
        //case 20:
        //    if (OptArgc < 1)
        //        ThrowError("Missing argument");
        //    strncpy(byte_148ef0, OptArgv[0], 13);
        //    byte_148ef0[12] = 0;
        //    bQuickStart = 1;
        //    byte_148eeb = 1;
        //    if (gGameOptions.gameType == 0)
        //        gGameOptions.gameType = 2;
        //    break;
        case 25:
            bNoDemo = 1;
            break;
        case 18:
            bQuickStart = 1;
            break;
        //case 12:
        //    EightyTwoFifty = 1;
        //    break;
        case 1:
            gPacketMode = PACKETMODE_2;
            break;
        case 21:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            strcpy(gPName, OptArgv[0]);
            bCustomName = 1;
            break;
        case 2:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            strcpy(gUserMapFilename, OptArgv[0]);
            bAddUserMap = 1;
            bNoDemo = 1;
            break;
        case 3:
            if (gSyncRate == 1)
                gPacketMode = PACKETMODE_2;
            else
                gPacketMode = PACKETMODE_1;
            break;
        case 4:
            //if (OptArgc < 1)
            //    ThrowError("Missing argument");
            //if (gGameOptions.nGameType == 0)
            //    gGameOptions.nGameType = 2;
            break;
        case 30:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            gNetPlayers = ClipRange(atoi(OptArgv[0]), 1, kMaxPlayers);
            gNetMode = NETWORK_SERVER;
            break;
        case 31:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            gNetMode = NETWORK_CLIENT;
            strncpy(gNetAddress, OptArgv[0], sizeof(gNetAddress)-1);
            break;
        case 14:
            gAutoAim = 0;
            break;
        case 22:
            bNoResend = 0;
            break;
        case 23:
            bSilentAim = 1;
            break;
        case 5:
            gGameOptions.nMonsterSettings = 0;
            break;
        case 6:
            if (OptArgc < 1)
                gDemo.SetupPlayback(NULL);
            else
                gDemo.SetupPlayback(OptArgv[0]);
            break;
        case 7:
            if (OptArgc < 1)
                gDemo.Create(NULL);
            else
                gDemo.Create(OptArgv[0]);
            break;
        case 8:
            gRobust = 1;
            break;
        case 13:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            levelOverrideINI(OptArgv[0]);
            bNoDemo = 1;
            break;
        case 26:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            pUserTiles = (char*)malloc(strlen(OptArgv[0])+1);
            if (!pUserTiles)
                return;
            strcpy(pUserTiles, OptArgv[0]);
            break;
        case 27:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            pUserSoundRFF = (char*)malloc(strlen(OptArgv[0])+1);
            if (!pUserSoundRFF)
                return;
            strcpy(pUserSoundRFF, OptArgv[0]);
            break;
        case 28:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            pUserRFF = (char*)malloc(strlen(OptArgv[0])+1);
            if (!pUserRFF)
                return;
            strcpy(pUserRFF, OptArgv[0]);
            break;
        case 9:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            strcpy(SetupFilename, OptArgv[0]);
            break;
        case 10:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            gSkill = strtoul(OptArgv[0], NULL, 0);
            if (gSkill < 0)
                gSkill = 0;
            else if (gSkill > 4)
                gSkill = 4;
            break;
        case 15:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            gSyncRate = ClipRange(strtoul(OptArgv[0], NULL, 0), 1, 4);
            if (gPacketMode == PACKETMODE_1)
                gSyncRate = 1;
            else if (gPacketMode == PACKETMODE_3)
                gSyncRate = 1;
            break;
        case -2:
        {
            const char *k = strrchr(OptFull, '.');
            if (k)
            {
                if (!Bstrcasecmp(k, ".map"))
                {
                    strcpy(gUserMapFilename, OptFull);
                    bAddUserMap = 1;
                    bNoDemo = 1;
                }
                else if (!Bstrcasecmp(k, ".grp") || !Bstrcasecmp(k, ".zip") || !Bstrcasecmp(k, ".pk3") || !Bstrcasecmp(k, ".pk4"))
                {
                    G_AddGroup(OptFull);
                }
                else if (!Bstrcasecmp(k, ".def"))
                {
                    clearDefNamePtr();
                    g_defNamePtr = dup_filename(OptFull);
                    initprintf("Using DEF file \"%s\".\n", g_defNamePtr);
                    continue;
                }
            }
            else
            {
                strcpy(gUserMapFilename, OptFull);
                bAddUserMap = 1;
                bNoDemo = 1;
            }
            break;
        }
        case 11:
            //bNoCDAudio = 1;
            break;
        case 32:
            initprintf("Autoload disabled\n");
            bNoAutoLoad = true;
            break;
        case 33:
            g_useCwd = true;
            break;
        case 34:
        {
            if (OptArgc < 1)
                ThrowError("Missing argument");
            uint32_t j = strtoul(OptArgv[0], NULL, 0);
            MAXCACHE1DSIZE = j<<10;
            initprintf("Cache size: %dkB\n", j);
            break;
        }
        case 35:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            G_AddGroup(OptArgv[0]);
            break;
        case 36:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            Bstrncpyz(g_modDir, OptArgv[0], sizeof(g_modDir));
            G_AddPath(OptArgv[0]);
            break;
        case 37:
            gCommandSetup = true;
            break;
        case 38:
            gNoSetup = true;
            gCommandSetup = false;
            break;
        case 39:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            gNetPort = strtoul(OptArgv[0], NULL, 0);
            break;
        case 40:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            G_AddDef(OptArgv[0]);
            break;
        case 41:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            G_AddDefModule(OptArgv[0]);
            break;
        case 42:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            G_AddPath(OptArgv[0]);
            break;
        }
    }
#if 0
    if (bAddUserMap)
    {
        char zNode[BMAX_PATH];
        char zDir[BMAX_PATH];
        char zFName[BMAX_PATH];
        _splitpath(gUserMapFilename, zNode, zDir, zFName, NULL);
        strcpy(g_modDir, zNode);
        strcat(g_modDir, zDir);
        strcpy(gUserMapFilename, zFName);
    }
#endif
}

void ClockStrobe()
{
    gGameClock++;
}

#if defined(_WIN32) && defined(DEBUGGINGAIDS)
// See FILENAME_CASE_CHECK in cache1d.c
static int32_t check_filename_casing(void)
{
    return 1;
}
#endif

int app_main(int argc, char const * const * argv)
{
    char buffer[BMAX_PATH];
    margc = argc;
    margv = argv;
#ifdef _WIN32
    if (!G_CheckCmdSwitch(argc, argv, "-noinstancechecking") && win_checkinstance())
    {
        if (!wm_ynbox(APPNAME, "Another Build game is currently running. "
                      "Do you wish to continue starting this copy?"))
            return 3;
    }

    backgroundidle = 0;

    G_ExtPreInit(argc, argv);

#ifdef DEBUGGINGAIDS
    extern int32_t (*check_filename_casing_fn)(void);
    check_filename_casing_fn = check_filename_casing;
#endif
#endif

    OSD_SetLogFile(APPBASENAME ".log");

    OSD_SetFunctions(NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     GAME_clearbackground,
                     BGetTime,
                     GAME_onshowosd);

    wm_setapptitle(APPNAME);

    initprintf(APPNAME " %s\n", s_buildRev);
    PrintBuildInfo();

    memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
    ParseOptions();
    G_ExtInit();

    if (!g_useCwd)
        G_AddSearchPaths();

    // used with binds for fast function lookup
    hash_init(&h_gamefuncs);
    for (bssize_t i=NUMGAMEFUNCTIONS-1; i>=0; i--)
    {
        if (gamefunctions[i][0] == '\0')
            continue;

        char *str = Bstrtolower(Xstrdup(gamefunctions[i]));
        hash_add(&h_gamefuncs,gamefunctions[i],i,0);
        hash_add(&h_gamefuncs,str,i,0);
        Bfree(str);
    }
    
#ifdef STARTUP_SETUP_WINDOW
    int const readSetup =
#endif
    CONFIG_ReadSetup();
    if (bCustomName)
        strcpy(szPlayerName, gPName);

    if (enginePreInit())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        ERRprintf("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
        Bexit(2);
    }

    if (Bstrcmp(SetupFilename, SETUPFILENAME))
        initprintf("Using config file \"%s\".\n", SetupFilename);

    ScanINIFiles();

#ifdef STARTUP_SETUP_WINDOW
    if (readSetup < 0 || (!gNoSetup && (configversion != BYTEVERSION || gSetup.forcesetup)) || gCommandSetup)
    {
        if (quitevent || !startwin_run())
        {
            engineUnInit();
            Bexit(0);
        }
    }
#endif

    G_LoadGroups(!bNoAutoLoad && !gSetup.noautoload);

    //if (!g_useCwd)
    //    G_CleanupSearchPaths();

    initprintf("Initializing OSD...\n");

    //Bsprintf(tempbuf, HEAD2 " %s", s_buildRev);
    OSD_SetVersion("Blood", 10, 0);
    OSD_SetParameters(0, 0, 0, 12, 2, 12, OSD_ERROR, OSDTEXT_RED, gamefunctions[gamefunc_Show_Console][0] == '\0' ? OSD_PROTECTED : 0);
    registerosdcommands();

    char *const setupFileName = Xstrdup(SetupFilename);
    char *const p = strtok(setupFileName, ".");

    if (!p || !Bstrcmp(SetupFilename, SETUPFILENAME))
        Bsprintf(buffer, "settings.cfg");
    else
        Bsprintf(buffer, "%s_settings.cfg", p);

    Bfree(setupFileName);

    OSD_Exec(buffer);
    OSD_Exec("autoexec.cfg");

    // Not neccessary ?
    // CONFIG_SetDefaultKeys(keydefaults, true);

    system_getcvars();

    Resource::heap = new QHeap(nMaxAlloc);
    gSysRes.Init(pUserRFF ? pUserRFF : "BLOOD.RFF");
    gGuiRes.Init("GUI.RFF");
    gSoundRes.Init(pUserSoundRFF ? pUserSoundRFF : "SOUNDS.RFF");


    { // Replace
        void qinitspritelists();
        int32_t qinsertsprite(int16_t nSector, int16_t nStat);
        int32_t qdeletesprite(int16_t nSprite);
        int32_t qchangespritesect(int16_t nSprite, int16_t nSector);
        int32_t qchangespritestat(int16_t nSprite, int16_t nStatus);
        animateoffs_replace = qanimateoffs;
        paletteLoadFromDisk_replace = qloadpalette;
        getpalookup_replace = qgetpalookup;
        initspritelists_replace = qinitspritelists;
        insertsprite_replace = qinsertsprite;
        deletesprite_replace = qdeletesprite;
        changespritesect_replace = qchangespritesect;
        changespritestat_replace = qchangespritestat;
        loadvoxel_replace = qloadvoxel;
        bloodhack = true;
    }

    initprintf("Initializing Build 3D engine\n");
    scrInit();
    
    initprintf("Loading tiles\n");
    if (pUserTiles)
    {
        strcpy(buffer,pUserTiles);
        strcat(buffer,"%03i.ART");
        if (!tileInit(0,buffer))
            ThrowError("User specified ART files not found");
    }
    else
    {
        if (!tileInit(0,NULL))
            ThrowError("TILES###.ART files not found");
    }

    LoadExtraArts();

    levelLoadDefaults();
    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        initprintf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }
    loaddefinitions_game(defsfile, FALSE);
    powerupInit();
    initprintf("Loading cosine table\n");
    trigInit(gSysRes);
    initprintf("Initializing view subsystem\n");
    viewInit();
    initprintf("Initializing dynamic fire\n");
    FireInit();
    initprintf("Initializing weapon animations\n");
    WeaponInit();
    LoadSaveSetup();
    LoadSavedInfo();
    gDemo.LoadDemoInfo();
    initprintf("There are %d demo(s) in the loop\n", gDemo.at59ef);
    initprintf("Loading control setup\n");
    ctrlInit();
    timerInit(120);
    timerSetCallback(ClockStrobe);
    // PORT-TODO: CD audio init

    initprintf("Initializing network users\n");
    netInitialize(true);
    scrSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp);
    scrSetGamma(gGamma);
    viewResizeView(gViewSize);
    initprintf("Initializing sound system\n");
    sndInit();
    sfxInit();
    gChoke.sub_83ff0(518, sub_84230);
    if (bAddUserMap)
    {
        levelAddUserMap(gUserMapFilename);
        gStartNewGame = 1;
    }
    SetupMenus();
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    if (!bQuickStart)
        credLogosDos();
    scrSetDac();
RESTART:
    sub_79760();
    gViewIndex = myconnectindex;
    gMe = gView = &gPlayer[myconnectindex];
    netBroadcastPlayerInfo(myconnectindex);
    initprintf("Waiting for network players!\n");
    netWaitForEveryone(0);
    if (gRestartGame)
    {
        // Network error
        gQuitGame = false;
        gRestartGame = false;
        netDeinitialize();
        netResetToSinglePlayer();
        goto RESTART;
    }
    UpdateNetworkMenus();
    if (!gDemo.at0 && gDemo.at59ef > 0 && gGameOptions.nGameType == 0 && !bNoDemo)
        gDemo.SetupPlayback(NULL);
    viewSetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
    gQuitGame = 0;
    gRestartGame = 0;
    if (gGameOptions.nGameType > 0)
    {
        KB_ClearKeysDown();
        KB_FlushKeyboardQueue();
        keyFlushScans();
    }
    else if (gDemo.at1 && !bAddUserMap && !bNoDemo)
        gDemo.Playback();
    if (gDemo.at59ef > 0)
        gGameMenuMgr.Deactivate();
    if (!bAddUserMap && !gGameStarted)
        gGameMenuMgr.Push(&menuMain, -1);
    ready2send = 1;
    while (!gQuitGame)
    {
        if (handleevents() && quitevent)
        {
            KB_KeyDown[sc_Escape] = 1;
            quitevent = 0;
        }
        netUpdate();
        MUSIC_Update();
        CONTROL_BindsEnabled = gInputMode == INPUT_MODE_0;
        switch (gInputMode)
        {
        case INPUT_MODE_1:
            if (gGameMenuMgr.m_bActive)
                gGameMenuMgr.Process();
            break;
        case INPUT_MODE_0:
            LocalKeys();
            break;
        default:
            break;
        }
        if (gQuitGame)
            continue;

        OSD_DispatchQueued();
        
        bool bDraw;
        if (gGameStarted)
        {
            char gameUpdate = false;
            double const gameUpdateStartTime = timerGetHiTicks();
            G_HandleAsync();
            while (gPredictTail < gNetFifoHead[myconnectindex] && !gPaused)
            {
                viewUpdatePrediction(&gFifoInput[gPredictTail&255][myconnectindex]);
            }
            if (numplayers == 1)
                gBufferJitter = 0;
            while (gGameClock >= gNetFifoClock && ready2send)
            {
                netGetInput();
                gNetFifoClock += 4;
                while (gNetFifoHead[myconnectindex]-gNetFifoTail > gBufferJitter && !gStartNewGame && !gQuitGame)
                {
                    int i;
                    for (i = connecthead; i >= 0; i = connectpoint2[i])
                        if (gNetFifoHead[i] == gNetFifoTail)
                            break;
                    if (i >= 0)
                        break;
                    faketimerhandler();
                    ProcessFrame();
                    timerUpdate();
                    gameUpdate = true;
                }
                timerUpdate();
            }
            if (gameUpdate)
            {
                g_gameUpdateTime = timerGetHiTicks() - gameUpdateStartTime;
                if (g_gameUpdateAvgTime < 0.f)
                    g_gameUpdateAvgTime = g_gameUpdateTime;
                g_gameUpdateAvgTime = ((GAMEUPDATEAVGTIMENUMSAMPLES-1.f)*g_gameUpdateAvgTime+g_gameUpdateTime)/((float) GAMEUPDATEAVGTIMENUMSAMPLES);
            }
            bDraw = viewFPSLimit() != 0;
            if (gQuitRequest && gQuitGame)
                videoClearScreen(0);
            else
            {
                netCheckSync();
                if (bDraw)
                {
                    viewDrawScreen();
                    g_gameUpdateAndDrawTime = timerGetHiTicks() - gameUpdateStartTime;
                }
            }
        }
        else
        {
            bDraw = viewFPSLimit() != 0;
            if (bDraw)
            {
                videoClearScreen(0);
                rotatesprite(160<<16,100<<16,65536,0,2518,0,0,0x4a,0,0,xdim-1,ydim-1);
            }
            G_HandleAsync();
            if (gQuitRequest && !gQuitGame)
                netBroadcastMyLogoff(gQuitRequest == 2);
        }
        if (bDraw)
        {
            switch (gInputMode)
            {
            case INPUT_MODE_1:
                if (gGameMenuMgr.m_bActive)
                    gGameMenuMgr.Draw();
                break;
            case INPUT_MODE_2:
                gPlayerMsg.ProcessKeys();
                gPlayerMsg.Draw();
                break;
            case INPUT_MODE_3:
                gEndGameMgr.ProcessKeys();
                gEndGameMgr.Draw();
                break;
            default:
                break;
            }
        }
        //scrNextPage();
        if (TestBitString(gotpic, 2342))
        {
            FireProcess();
            ClearBitString(gotpic, 2342);
        }
        //if (byte_148e29 && gStartNewGame)
        //{
        //	gStartNewGame = 0;
        //	gQuitGame = 1;
        //}
        if (gStartNewGame)
            StartLevel(&gGameOptions);
    }
    ready2send = 0;
    if (gDemo.at0)
        gDemo.Close();
    if (gRestartGame)
    {
        UpdateDacs(0, true);
        sndStopSong();
        FX_StopAllSounds();
        gQuitGame = 0;
        gQuitRequest = 0;
        gRestartGame = 0;
        gGameStarted = 0;
        levelSetupOptions(0,0);
        while (gGameMenuMgr.m_bActive)
        {
            gGameMenuMgr.Process();
            G_HandleAsync();
            if (viewFPSLimit())
            {
                videoClearScreen(0);
                gGameMenuMgr.Draw();
            }
        }
        if (gGameOptions.nGameType != 0)
        {
            if (!gDemo.at0 && gDemo.at59ef > 0 && gGameOptions.nGameType == 0 && !bNoDemo)
                gDemo.NextDemo();
            videoSetViewableArea(0,0,xdim-1,ydim-1);
            if (!bQuickStart)
                credLogosDos();
            scrSetDac();
        }
        goto RESTART;
    }
    ShutDown();

    return 0;
}

static int32_t S_DefineAudioIfSupported(char *fn, const char *name)
{
#if !defined HAVE_FLAC || !defined HAVE_VORBIS
    const char *extension = Bstrrchr(name, '.');
# if !defined HAVE_FLAC
    if (extension && !Bstrcasecmp(extension, ".flac"))
        return -2;
# endif
# if !defined HAVE_VORBIS
    if (extension && !Bstrcasecmp(extension, ".ogg"))
        return -2;
# endif
#endif
    Bstrncpy(fn, name, BMAX_PATH);
    return 0;
}

// Returns:
//   0: all OK
//  -1: ID declaration was invalid:
static int32_t S_DefineMusic(const char *ID, const char *name)
{
    int32_t sel = MUS_FIRST_SPECIAL;

    Bassert(ID != NULL);

    if (!Bstrcmp(ID,"intro"))
    {
        sel = MUS_INTRO;
    }
    else if (!Bstrcmp(ID,"loading"))
    {
        sel = MUS_LOADING;
    }
    else
    {
        sel = levelGetMusicIdx(ID);
        if (sel < 0)
            return -1;
    }

    int nEpisode = sel/kMaxLevels;
    int nLevel = sel%kMaxLevels;
    return S_DefineAudioIfSupported(gEpisodeInfo[nEpisode].at28[nLevel].atd0, name);
}

static int parsedefinitions_game(scriptfile *, int);

static void parsedefinitions_game_include(const char *fileName, scriptfile *pScript, const char *cmdtokptr, int const firstPass)
{
    scriptfile *included = scriptfile_fromfile(fileName);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || pScript == NULL) // this is a bit overboard to prevent unused parameter warnings
            {
           // initprintf("Warning: Failed including %s as module\n", fn);
            }
/*
        else
            {
            initprintf("Warning: Failed including %s on line %s:%d\n",
                       fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
            }
*/
    }
    else
    {
        parsedefinitions_game(included, firstPass);
        scriptfile_close(included);
    }
}

#if 0
static void parsedefinitions_game_animsounds(scriptfile *pScript, const char * blockEnd, char const * fileName, dukeanim_t * animPtr)
{
    Bfree(animPtr->sounds);

    size_t numPairs = 0, allocSize = 4;

    animPtr->sounds = (animsound_t *)Xmalloc(allocSize * sizeof(animsound_t));
    animPtr->numsounds = 0;

    int defError = 1;
    uint16_t lastFrameNum = 1;

    while (pScript->textptr < blockEnd)
    {
        int32_t frameNum;
        int32_t soundNum;

        // HACK: we've reached the end of the list
        //  (hack because it relies on knowledge of
        //   how scriptfile_* preprocesses the text)
        if (blockEnd - pScript->textptr == 1)
            break;

        // would produce error when it encounters the closing '}'
        // without the above hack
        if (scriptfile_getnumber(pScript, &frameNum))
            break;

        defError = 1;

        if (scriptfile_getsymbol(pScript, &soundNum))
            break;

        // frame numbers start at 1 for us
        if (frameNum <= 0)
        {
            initprintf("Error: frame number must be greater zero on line %s:%d\n", pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        if (frameNum < lastFrameNum)
        {
            initprintf("Error: frame numbers must be in (not necessarily strictly)"
                       " ascending order (line %s:%d)\n",
                       pScript->filename, scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        lastFrameNum = frameNum;

        if ((unsigned)soundNum >= MAXSOUNDS && soundNum != -1)
        {
            initprintf("Error: sound number #%d invalid on line %s:%d\n", soundNum, pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        if (numPairs >= allocSize)
        {
            allocSize *= 2;
            animPtr->sounds = (animsound_t *)Xrealloc(animPtr->sounds, allocSize * sizeof(animsound_t));
        }

        defError = 0;

        animsound_t & sound = animPtr->sounds[numPairs];
        sound.frame = frameNum;
        sound.sound = soundNum;

        ++numPairs;
    }

    if (!defError)
    {
        animPtr->numsounds = numPairs;
        // initprintf("Defined sound sequence for hi-anim \"%s\" with %d frame/sound pairs\n",
        //           hardcoded_anim_tokens[animnum].text, numpairs);
    }
    else
    {
        DO_FREE_AND_NULL(animPtr->sounds);
        initprintf("Failed defining sound sequence for anim \"%s\".\n", fileName);
    }
}

#endif

static int parsedefinitions_game(scriptfile *pScript, int firstPass)
{
    int   token;
    char *pToken;

    static const tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "includedefault",  T_INCLUDEDEFAULT   },
        { "#includedefault", T_INCLUDEDEFAULT   },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautoload",      T_NOAUTOLOAD       },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },
        //{ "cutscene",        T_CUTSCENE         },
        //{ "animsounds",      T_ANIMSOUNDS       },
        { "renamefile",      T_RENAMEFILE       },
        { "globalgameflags", T_GLOBALGAMEFLAGS  },
        { "rffdefineid",     T_RFFDEFINEID      },
    };

    static const tokenlist soundTokens[] =
    {
        { "id",       T_ID },
        { "file",     T_FILE },
        { "minpitch", T_MINPITCH },
        { "maxpitch", T_MAXPITCH },
        { "priority", T_PRIORITY },
        { "type",     T_TYPE },
        { "distance", T_DISTANCE },
        { "volume",   T_VOLUME },
    };

#if 0
    static const tokenlist animTokens [] =
    {
        { "delay",         T_DELAY },
        { "aspect",        T_ASPECT },
        { "sounds",        T_SOUND },
        { "forcefilter",   T_FORCEFILTER },
        { "forcenofilter", T_FORCENOFILTER },
        { "texturefilter", T_TEXTUREFILTER },
    };
#endif

    do
    {
        token  = getatoken(pScript, tokens, ARRAY_SIZE(tokens));
        pToken = pScript->ltextptr;

        switch (token)
        {
        case T_LOADGRP:
        {
            char *fileName;

            pathsearchmode = 1;
            if (!scriptfile_getstring(pScript,&fileName) && firstPass)
            {
                if (initgroupfile(fileName) == -1)
                    initprintf("Could not find file \"%s\".\n", fileName);
                else
                {
                    initprintf("Using file \"%s\" as game data.\n", fileName);
                    if (!bNoAutoLoad && !gSetup.noautoload)
                        G_DoAutoload(fileName);
                }
            }

            pathsearchmode = 0;
        }
        break;
        case T_CACHESIZE:
        {
            int32_t cacheSize;

            if (scriptfile_getnumber(pScript, &cacheSize) || !firstPass)
                break;

            if (cacheSize > 0)
                MAXCACHE1DSIZE = cacheSize << 10;
        }
        break;
        case T_INCLUDE:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript, &fileName))
                parsedefinitions_game_include(fileName, pScript, pToken, firstPass);

            break;
        }
        case T_INCLUDEDEFAULT:
        {
            parsedefinitions_game_include(G_DefaultDefFile(), pScript, pToken, firstPass);
            break;
        }
        case T_NOAUTOLOAD:
            if (firstPass)
                bNoAutoLoad = true;
            break;
        case T_MUSIC:
        {
            char *tokenPtr = pScript->ltextptr;
            char *musicID  = NULL;
            char *fileName = NULL;
            char *musicEnd;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID: scriptfile_getstring(pScript, &musicID); break;
                    case T_FILE: scriptfile_getstring(pScript, &fileName); break;
                }
            }

            if (!firstPass)
            {
                if (musicID==NULL)
                {
                    initprintf("Error: missing ID for music definition near line %s:%d\n",
                               pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || check_file_exist(fileName))
                    break;

                if (S_DefineMusic(musicID, fileName) == -1)
                    initprintf("Error: invalid music ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
            }
        }
        break;

        case T_RFFDEFINEID:
        {
            char *resName = NULL;
            char *resType = NULL;
            char *rffName = NULL;
            int resID;

            if (scriptfile_getstring(pScript, &resName))
                break;

            if (scriptfile_getstring(pScript, &resType))
                break;

            if (scriptfile_getnumber(pScript, &resID))
                break;

            if (scriptfile_getstring(pScript, &rffName))
                break;

            if (!firstPass)
            {
                if (!Bstrcasecmp(rffName, "SYSTEM"))
                    gSysRes.AddExternalResource(resName, resType, resID);
                else if (!Bstrcasecmp(rffName, "SOUND"))
                    gSoundRes.AddExternalResource(resName, resType, resID);
            }
        }
        break;

#if 0
        case T_CUTSCENE:
        {
            char *fileName = NULL;

            scriptfile_getstring(pScript, &fileName);

            char *animEnd;

            if (scriptfile_getbraces(pScript, &animEnd))
                break;

            if (!firstPass)
            {
                dukeanim_t *animPtr = Anim_Find(fileName);

                if (!animPtr)
                {
                    animPtr = Anim_Create(fileName);
                    animPtr->framedelay = 10;
                    animPtr->frameflags = 0;
                }

                int32_t temp;

                while (pScript->textptr < animEnd)
                {
                    switch (getatoken(pScript, animTokens, ARRAY_SIZE(animTokens)))
                    {
                        case T_DELAY:
                            scriptfile_getnumber(pScript, &temp);
                            animPtr->framedelay = temp;
                            break;
                        case T_ASPECT:
                        {
                            double dtemp, dtemp2;
                            scriptfile_getdouble(pScript, &dtemp);
                            scriptfile_getdouble(pScript, &dtemp2);
                            animPtr->frameaspect1 = dtemp;
                            animPtr->frameaspect2 = dtemp2;
                            break;
                        }
                        case T_SOUND:
                        {
                            char *animSoundsEnd = NULL;
                            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                                break;
                            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
                            break;
                        }
                        case T_FORCEFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCEFILTER;
                            break;
                        case T_FORCENOFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCENOFILTER;
                            break;
                        case T_TEXTUREFILTER:
                            animPtr->frameflags |= CUTSCENE_TEXTUREFILTER;
                            break;
                    }
                }
            }
            else
                pScript->textptr = animEnd;
        }
        break;
        case T_ANIMSOUNDS:
        {
            char *tokenPtr     = pScript->ltextptr;
            char *fileName     = NULL;

            scriptfile_getstring(pScript, &fileName);
            if (!fileName)
                break;

            char *animSoundsEnd = NULL;

            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                break;

            if (firstPass)
            {
                pScript->textptr = animSoundsEnd;
                break;
            }

            dukeanim_t *animPtr = Anim_Find(fileName);

            if (!animPtr)
            {
                initprintf("Error: expected animation filename on line %s:%d\n",
                    pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
                break;
            }

            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
        }
        break;
        case T_SOUND:
        {
            char *tokenPtr = pScript->ltextptr;
            char *fileName = NULL;
            char *musicEnd;

            double volume = 1.0;

            int32_t soundNum = -1;
            int32_t maxpitch = 0;
            int32_t minpitch = 0;
            int32_t priority = 0;
            int32_t type     = 0;
            int32_t distance = 0;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID:       scriptfile_getsymbol(pScript, &soundNum); break;
                    case T_FILE:     scriptfile_getstring(pScript, &fileName); break;
                    case T_MINPITCH: scriptfile_getsymbol(pScript, &minpitch); break;
                    case T_MAXPITCH: scriptfile_getsymbol(pScript, &maxpitch); break;
                    case T_PRIORITY: scriptfile_getsymbol(pScript, &priority); break;
                    case T_TYPE:     scriptfile_getsymbol(pScript, &type);     break;
                    case T_DISTANCE: scriptfile_getsymbol(pScript, &distance); break;
                    case T_VOLUME:   scriptfile_getdouble(pScript, &volume);   break;
                }
            }

            if (!firstPass)
            {
                if (soundNum==-1)
                {
                    initprintf("Error: missing ID for sound definition near line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || check_file_exist(fileName))
                    break;

                // maybe I should have just packed this into a sound_t and passed a reference...
                if (S_DefineSound(soundNum, fileName, minpitch, maxpitch, priority, type, distance, volume) == -1)
                    initprintf("Error: invalid sound ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
            }
        }
        break;
#endif
        case T_GLOBALGAMEFLAGS: scriptfile_getnumber(pScript, &blood_globalflags); break;
        case T_EOF: return 0;
        default: break;
        }
    }
    while (1);

    return 0;
}

int loaddefinitions_game(const char *fileName, int32_t firstPass)
{
    scriptfile *pScript = scriptfile_fromfile(fileName);

    if (pScript)
        parsedefinitions_game(pScript, firstPass);

    for (char const * m : g_defModules)
        parsedefinitions_game_include(m, NULL, "null", firstPass);

    if (pScript)
        scriptfile_close(pScript);

    scriptfile_clearsymbols();

    return 0;
}

INICHAIN *pINIChain;
INICHAIN const*pINISelected;
int nINICount = 0;

const char *pzCrypticArts[] = {
    "CPART07.AR_", "CPART15.AR_"
};

INIDESCRIPTION gINIDescription[] = {
    { "BLOOD: One Unit Whole Blood", "BLOOD.INI", NULL, 0 },
    { "Cryptic passage", "CRYPTIC.INI", pzCrypticArts, ARRAY_SSIZE(pzCrypticArts) },
};

bool AddINIFile(const char *pzFile, bool bForce = false)
{
    char *pzFN;
    struct Bstat st;
    static INICHAIN *pINIIter = NULL;
    if (!bForce)
    {
        if (findfrompath(pzFile, &pzFN)) return false; // failed to resolve the filename
        if (Bstat(pzFN, &st))
        {
            Bfree(pzFN);
            return false;
        } // failed to stat the file
        Bfree(pzFN);
        IniFile *pTempIni = new IniFile(pzFile);
        if (!pTempIni->FindSection("Episode1"))
        {
            delete pTempIni;
            return false;
        }
        delete pTempIni;
    }
    if (!pINIChain)
        pINIIter = pINIChain = new INICHAIN;
    else
        pINIIter = pINIIter->pNext = new INICHAIN;
    pINIIter->pNext = NULL;
    pINIIter->pDescription = NULL;
    Bstrncpy(pINIIter->zName, pzFile, BMAX_PATH);
    for (int i = 0; i < ARRAY_SSIZE(gINIDescription); i++)
    {
        if (!Bstrncasecmp(pINIIter->zName, gINIDescription[i].pzFilename, BMAX_PATH))
        {
            pINIIter->pDescription = &gINIDescription[i];
            break;
        }
    }
    return true;
}

void ScanINIFiles(void)
{
    nINICount = 0;
    CACHE1D_FIND_REC *pINIList = klistpath("/", "*.ini", CACHE1D_FIND_FILE);
    pINIChain = NULL;

    if (bINIOverride || !pINIList)
    {
        AddINIFile(BloodIniFile, true);
    }

    for (auto pIter = pINIList; pIter; pIter = pIter->next)
    {
        AddINIFile(pIter->name);
    }
    klistfree(pINIList);
    pINISelected = pINIChain;
    for (auto pIter = pINIChain; pIter; pIter = pIter->pNext)
    {
        if (!Bstrncasecmp(BloodIniFile, pIter->zName, BMAX_PATH))
        {
            pINISelected = pIter;
            break;
        }
    }
}

bool LoadArtFile(const char *pzFile)
{
    int hFile = kopen4loadfrommod(pzFile, 0);
    if (hFile == -1)
    {
        initprintf("Can't open extra art file:\"%s\"\n", pzFile);
        return false;
    }
    artheader_t artheader;
    int nStatus = artReadHeader(hFile, pzFile, &artheader);
    if (nStatus != 0)
    {
        kclose(hFile);
        initprintf("Error reading extra art file:\"%s\"\n", pzFile);
        return false;
    }
    for (int i = artheader.tilestart; i <= artheader.tileend; i++)
        tileDelete(i);
    artReadManifest(hFile, &artheader);
    artPreloadFile(hFile, &artheader);
    for (int i = artheader.tilestart; i <= artheader.tileend; i++)
        tileUpdatePicSiz(i);
    kclose(hFile);
    return true;
}

void LoadExtraArts(void)
{
    if (!pINISelected->pDescription)
        return;
    for (int i = 0; i < pINISelected->pDescription->nArts; i++)
    {
        LoadArtFile(pINISelected->pDescription->pzArts[i]);
    }
}

bool DemoRecordStatus(void) {
    return gDemo.at0;
}

bool VanillaMode() {
    return gDemo.m_bLegacy && gDemo.at1;
}

bool fileExistsRFF(int id, const char *ext) {
    return gSysRes.Lookup(id, ext);
}