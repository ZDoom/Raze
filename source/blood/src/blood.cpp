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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "mmulti.h"
#include "compat.h"
#include "baselayer.h"
#include "common.h"
#include "common_game.h"

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
#include "gib.h"
#include "getopt.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "gamemenu.h"
#include "mirrors.h"
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
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "statistics.h"
#include "menu.h"
#include "raze_sound.h"
#include "nnexts.h"
#include"secrets.h"

BEGIN_BLD_NS


INPUT_MODE gInputMode;

#ifdef USE_QHEAP
unsigned int nMaxAlloc = 0x4000000;
#endif

char bAddUserMap = false;
bool bNoDemo = false;
bool bQuickStart = true;


char gUserMapFilename[BMAX_PATH];

short BloodVersion = 0x115;

int gNetPlayers;

char *pUserTiles = NULL;

int gChokeCounter = 0;

double g_gameUpdateTime, g_gameUpdateAndDrawTime;
double g_gameUpdateAvgTime = 0.001;

bool gQuitGame;
int gQuitRequest;

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
    T_TILEFROMTEXTURE,
    T_IFCRC, T_IFMATCH, T_CRC32,
    T_SIZE,
    T_SURFACE,
    T_VOXEL,
    T_VIEW,
    T_SHADE,
};

int blood_globalflags;

void ShutDown(void)
{
    if (!in3dmode())
        return;
	netDeinitialize();
    //sndTerm();
    sfxTerm();
    // PORT_TODO: Check argument
    DO_FREE_AND_NULL(pUserTiles);
}

void QuitGame(void)
{
    ShutDown();
    Bexit(0);
}

void PrecacheDude(spritetype *pSprite)
{
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    seqPrecacheId(pDudeInfo->seqStartID);
    seqPrecacheId(pDudeInfo->seqStartID+5);
    seqPrecacheId(pDudeInfo->seqStartID+1);
    seqPrecacheId(pDudeInfo->seqStartID+2);
    switch (pSprite->type)
    {
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        seqPrecacheId(pDudeInfo->seqStartID+13);
        seqPrecacheId(pDudeInfo->seqStartID+14);
        seqPrecacheId(pDudeInfo->seqStartID+15);
        break;
    case kDudeZombieButcher:
    case kDudeGillBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        seqPrecacheId(pDudeInfo->seqStartID+10);
        seqPrecacheId(pDudeInfo->seqStartID+11);
        break;
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+6);
        fallthrough__;
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        break;
    case kDudePhantasm:
    case kDudeHellHound:
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
    case kDudeSpiderMother:
    case kDudeTchernobog:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        break;
    case kDudeCerberusTwoHead:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        fallthrough__;
    case kDudeHand:
    case kDudeBoneEel:
    case kDudeBat:
    case kDudeRat:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        break;
    case kDudeCultistBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        break;
    case kDudeZombieAxeBuried:
        seqPrecacheId(pDudeInfo->seqStartID+12);
        seqPrecacheId(pDudeInfo->seqStartID+9);
        fallthrough__;
    case kDudeZombieAxeLaying:
        seqPrecacheId(pDudeInfo->seqStartID+10);
        fallthrough__;
    case kDudeZombieAxeNormal:
        seqPrecacheId(pDudeInfo->seqStartID+6);
        seqPrecacheId(pDudeInfo->seqStartID+7);
        seqPrecacheId(pDudeInfo->seqStartID+8);
        seqPrecacheId(pDudeInfo->seqStartID+11);
        seqPrecacheId(pDudeInfo->seqStartID+13);
        seqPrecacheId(pDudeInfo->seqStartID+14);
        break;
    }
}

void PrecacheThing(spritetype *pSprite) {
    switch (pSprite->type) {
        case kThingGlassWindow: // worthless...
        case kThingFluorescent:
            seqPrecacheId(12);
            break;
        case kThingSpiderWeb:
            seqPrecacheId(15);
            break;
        case kThingMetalGrate:
            seqPrecacheId(21);
            break;
        case kThingFlammableTree:
            seqPrecacheId(25);
            seqPrecacheId(26);
            break;
        case kTrapMachinegun:
            seqPrecacheId(38);
            seqPrecacheId(40);
            seqPrecacheId(28);
            break;
        case kThingObjectGib:
        //case kThingObjectExplode: weird that only gib object is precached and this one is not
            break;
    }
    tilePrecacheTile(pSprite->picnum);
}

void PreloadTiles(void)
{
    nPrecacheCount = 0;
    int skyTile = -1;
    memset(gotpic,0,sizeof(gotpic));
    // Fonts
    for (int i = 0; i < kFontNum; i++)
    {
        for (int j = 0; j < 96; j++)
        {
            tilePrecacheTile(gFont[i].tile + j, 0);
        }
    }
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
            case kStatDude:
                PrecacheDude(pSprite);
                break;
            case kStatThing:
                PrecacheThing(pSprite);
                break;
            default:
                tilePrecacheTile(pSprite->picnum);
                break;
            }
        }
    }

    // Precache common SEQs
    for (int i = 0; i < 100; i++)
    {
        seqPrecacheId(i);
    }

    tilePrecacheTile(1147); // water drip
    tilePrecacheTile(1160); // blood drip

    // Player SEQs
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

    if (skyTile > -1 && skyTile < kMaxTiles)
    {
        for (int i = 1; i < gSkyCount; i++)
            tilePrecacheTile(skyTile+i, 0);
    }

    WeaponPrecache();
    viewPrecacheTiles();
    fxPrecache();
    gibPrecache();

    gameHandleEvents();
}

void PreloadCache(void)
{
    if (gDemo.at1)
        return;
    PreloadTiles();
    ClockTicks clock = totalclock;
    int cnt = 0;
    int percentDisplayed = -1;

    for (int i=0; i<kMaxTiles && !inputState.GetKeyStatus(sc_Space); i++)
    {
        if (TestBitString(gotpic, i))
        {
			// For the hardware renderer precaching the raw pixel data is pointless.
			if (videoGetRenderMode() < REND_POLYMOST)
				tileLoad(i);

#ifdef USE_OPENGL
            if (r_precache) PrecacheHardwareTextures(i);
#endif

            if ((++cnt & 7) == 0)
                gameHandleEvents();

#if 0
            if (videoGetRenderMode() != REND_CLASSIC && totalclock - clock > (kTicRate>>2))
            {
                int const percentComplete = min(100, tabledivide32_noinline(100 * cnt, nPrecacheCount));

                // this just prevents the loading screen percentage bar from making large jumps
                while (percentDisplayed < percentComplete)
                {
                    gameHandleEvents();
                    Bsprintf(tempbuf, "Loaded %d%% (%d/%d textures)\n", percentDisplayed, cnt, nPrecacheCount);
                    viewLoadingScreenUpdate(tempbuf, percentDisplayed);
                    videoNextPage();

                    if (totalclock - clock >= 1)
                    {
                        clock = totalclock;
                        percentDisplayed++;
                    }
                }

                clock = totalclock;
            }
#endif
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

int G_TryMapHack(const char* mhkfile)
{
    int const failure = engineLoadMHK(mhkfile);

    if (!failure)
        Printf("Loaded map hack file \"%s\"\n", mhkfile);

    return failure;
}

void G_LoadMapHack(char* outbuf, const char* filename)
{
    if (filename != NULL)
        Bstrcpy(outbuf, filename);

    append_ext_UNSAFE(outbuf, ".mhk");

    if (G_TryMapHack(outbuf) && usermaphacks != NULL)
    {
        auto pMapInfo = (usermaphack_t*)bsearch(&g_loadedMapHack, usermaphacks, num_usermaphacks,
            sizeof(usermaphack_t), compare_usermaphacks);
        if (pMapInfo)
            G_TryMapHack(pMapInfo->mhkfile);
    }
}

#ifdef POLYMER
void G_RefreshLights(void)
{
    if (Numsprites && videoGetRenderMode() == REND_POLYMER)
    {
        int statNum = 0;

        do
        {
            int spriteNum = headspritestat[statNum++];

            while (spriteNum >= 0)
            {
                actDoLight(spriteNum);
                spriteNum = nextspritestat[spriteNum];
            }
        }
        while (statNum < MAXSTATUS);
    }
}
#endif // POLYMER


PLAYER gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];

vec3_t startpos;
int16_t startang, startsectnum;

void StartLevel(GAMEOPTIONS *gameOptions)
{
	STAT_Update(0);
    EndLevel();
    gInput = {};
    gStartNewGame = 0;
    ready2send = 0;
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

        ///////
        gGameOptions.weaponsV10x = gWeaponsV10x;
        ///////
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
        gGameOptions.bFriendlyFire = gPacketStartGame.bFriendlyFire;
        gGameOptions.bKeepKeysOnRespawn = gPacketStartGame.bKeepKeysOnRespawn;
        if (gPacketStartGame.userMap)
            levelAddUserMap(gPacketStartGame.userMapName);
        else
            levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);

        ///////
        gGameOptions.weaponsV10x = gPacketStartGame.weaponsV10x;
        ///////

        gBlueFlagDropped = false;
        gRedFlagDropped = false;
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
    enginecompatibility_mode = ENGINECOMPATIBILITY_19960925;//bVanilla;
    memset(xsprite,0,sizeof(xsprite));
    memset(sprite,0,kMaxSprites*sizeof(spritetype));
    drawLoadingScreen();
    if (dbLoadMap(gameOptions->zLevelName,(int*)&startpos.x,(int*)&startpos.y,(int*)&startpos.z,&startang,&startsectnum,(unsigned int*)&gameOptions->uMapCRC))
    {
        gQuitGame = true;
        return;
    }
    char levelName[BMAX_PATH];
    currentLevel = &mapList[gGameOptions.nEpisode * kMaxLevels + gGameOptions.nLevel];
    SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
	STAT_NewLevel(currentLevel->fileName);
    G_LoadMapHack(levelName, gameOptions->zLevelName);
    wsrand(gameOptions->uMapCRC);
    gKillMgr.Clear();
    gSecretMgr.Clear();
    gLevelTime = 0;
    automapping = 1;
  
    int modernTypesErased = 0;
    for (int i = 0; i < kMaxSprites; i++)
    {
        spritetype *pSprite = &sprite[i];
        if (pSprite->statnum < kMaxStatus && pSprite->extra > 0) {
            
            XSPRITE *pXSprite = &xsprite[pSprite->extra];
            if ((pXSprite->lSkill & (1 << gameOptions->nDifficulty)) || (pXSprite->lS && gameOptions->nGameType == 0)
                || (pXSprite->lB && gameOptions->nGameType == 2) || (pXSprite->lT && gameOptions->nGameType == 3)
                || (pXSprite->lC && gameOptions->nGameType == 1)) {
                
                DeleteSprite(i);
                continue;
            }

            
            #ifdef NOONE_EXTENSIONS
            if (!gModernMap && nnExtEraseModernStuff(pSprite, pXSprite))
               modernTypesErased++;
            #endif
                }
                }
                
    #ifdef NOONE_EXTENSIONS
    if (!gModernMap)
        Printf("> Modern types erased: %d.\n", modernTypesErased);
    #endif

    startpos.z = getflorzofslope(startsectnum,startpos.x,startpos.y);
    for (int i = 0; i < kMaxPlayers; i++) {
        gStartZone[i].x = startpos.x;
        gStartZone[i].y = startpos.y;
        gStartZone[i].z = startpos.z;
        gStartZone[i].sectnum = startsectnum;
        gStartZone[i].ang = startang;

        #ifdef NOONE_EXTENSIONS
        // Create spawn zones for players in teams mode.
        if (gModernMap && i <= kMaxPlayers / 2) {
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
        #endif
    }
    InitSectorFX();
    warpInit();
    actInit(false);
    evInit();
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (!(gameOptions->uGameFlags&1))
        {
            if (numplayers == 1)
            {
                gProfile[i].skill = gSkill;
                gProfile[i].nAutoAim = cl_autoaim;
                gProfile[i].nWeaponSwitch = cl_weaponswitch;
            }
            playerInit(i,0);
        }
        playerStart(i, 1);
    }
    if (gameOptions->uGameFlags&1)
    {
        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            PLAYER *pPlayer = &gPlayer[i];
            pPlayer->pXSprite->health &= 0xf000;
            pPlayer->pXSprite->health |= gHealthTemp[i];
            pPlayer->weaponQav = gPlayerTemp[i].weaponQav;
            pPlayer->curWeapon = gPlayerTemp[i].curWeapon;
            pPlayer->weaponState = gPlayerTemp[i].weaponState;
            pPlayer->weaponAmmo = gPlayerTemp[i].weaponAmmo;
            pPlayer->qavCallback = gPlayerTemp[i].qavCallback;
            pPlayer->qavLoop = gPlayerTemp[i].qavLoop;
            pPlayer->weaponTimer = gPlayerTemp[i].weaponTimer;
            pPlayer->nextWeapon = gPlayerTemp[i].nextWeapon;
        }
    }
    gameOptions->uGameFlags &= ~3;
    PreloadCache();
    InitMirrors();
    gFrameClock = 0;
    trInit();
    if (!bVanilla && !gMe->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
        sfxSetReverb(0);
    ambInit();
    sub_79760();
    gFrame = 0;
    gChokeCounter = 0;
	if (!gDemo.at1)
		M_ClearMenus();
    levelTryPlayMusicOrNothing(gGameOptions.nEpisode, gGameOptions.nLevel);
    // viewSetMessage("");
    viewSetErrorMessage("");
    viewResizeView(gViewSize);
    if (gGameOptions.nGameType == 3)
        gGameMessageMgr.SetCoordinates(gViewX0S+1,gViewY0S+15);
    netWaitForEveryone(0);
    totalclock = 0;
    paused = 0;
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
        gGameOptions.bFriendlyFire = gPacketStartGame.bFriendlyFire;
        gGameOptions.bKeepKeysOnRespawn = gPacketStartGame.bKeepKeysOnRespawn;
        
        ///////
        gGameOptions.weaponsV10x = gPacketStartGame.weaponsV10x;
        ///////

        gBlueFlagDropped = false;
        gRedFlagDropped = false;

        if (gPacketStartGame.userMap)
            levelAddUserMap(gPacketStartGame.userMapName);
        else
            levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    }
    StartLevel(&gGameOptions);
}


void LocalKeys(void)
{
    bool alt = inputState.AltPressed();
    bool ctrl = inputState.CtrlPressed();
    bool shift = inputState.ShiftPressed();
    if (buttonMap.ButtonDown(gamefunc_Third_Person_View) && !alt && !shift)
    {
        buttonMap.ClearButton(gamefunc_Third_Person_View);
        if (gViewPos > VIEWPOS_0)
            gViewPos = VIEWPOS_0;
        else
            gViewPos = VIEWPOS_1;
    }
    if (buttonMap.ButtonDown(gamefunc_See_Coop_View))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
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
                if (oldViewIndex == gViewIndex || gMe->teamId == gPlayer[gViewIndex].teamId)
                    break;
            } while (oldViewIndex != gViewIndex);
            gView = &gPlayer[gViewIndex];
        }
    }
    char key;
    if ((key = inputState.keyGetScan()) != 0)
    {
        if ((alt || shift) && gGameOptions.nGameType > 0 && key >= sc_F1 && key <= sc_F10)
        {
            char fk = key - sc_F1;
            if (alt)
            {
                netBroadcastTaunt(myconnectindex, fk);
            }
            else
            {
                gPlayerMsg.Set(*CombatMacros[fk]);
                gPlayerMsg.Send();
            }
            buttonMap.ClearButton(gamefunc_Third_Person_View);
            return;
        }
#if 0
        switch (key)
        {
        case sc_kpad_Period:
        case sc_Delete:
            if (ctrl && alt)
            {
                gQuitGame = 1;  // uh, what?
                return;
            }
            break;
        case default:
            break;
        }
#endif
    }
}

bool gRestartGame = false;

void ProcessFrame(void)
{
    char buffer[128];
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        gPlayer[i].input.buttonFlags = gFifoInput[gNetFifoTail&255][i].buttonFlags;
        gPlayer[i].input.keyFlags.word |= gFifoInput[gNetFifoTail&255][i].keyFlags.word;
        gPlayer[i].input.useFlags.byte |= gFifoInput[gNetFifoTail&255][i].useFlags.byte;
        if (gFifoInput[gNetFifoTail&255][i].newWeapon)
            gPlayer[i].input.newWeapon = gFifoInput[gNetFifoTail&255][i].newWeapon;
        gPlayer[i].input.forward = gFifoInput[gNetFifoTail&255][i].forward;
        gPlayer[i].input.q16turn = gFifoInput[gNetFifoTail&255][i].q16turn;
        gPlayer[i].input.strafe = gFifoInput[gNetFifoTail&255][i].strafe;
        gPlayer[i].input.q16mlook = gFifoInput[gNetFifoTail&255][i].q16mlook;
    }
    gNetFifoTail++;
    if (!(gFrame&7))
    {
        CalcGameChecksum();
        memcpy(gCheckFifo[gCheckHead[myconnectindex]&255][myconnectindex], gChecksum, sizeof(gChecksum));
        gCheckHead[myconnectindex]++;
    }
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (gPlayer[i].input.keyFlags.quit)
        {
            gPlayer[i].input.keyFlags.quit = 0;
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
        if (gPlayer[i].input.keyFlags.restart)
        {
            gPlayer[i].input.keyFlags.restart = 0;
            levelRestart();
            return;
        }
        if (gPlayer[i].input.keyFlags.pause)
        {
            gPlayer[i].input.keyFlags.pause = 0;
            if (paused && gGameOptions.nGameType > 0 && numplayers > 1)
            {
                sprintf(buffer,"%s paused the game",gProfile[i].name);
                viewSetMessage(buffer);
            }
        }
    }
    viewClearInterpolations();
    if (!gDemo.at1)
    {
        if (paused || gEndGameMgr.at0 || (gGameOptions.nGameType == 0 && M_Active()))
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
    evProcess((int)gFrameClock);
    seqProcess(4);
    DoSectorPanning();
    actProcessSprites();
    actPostProcess();
#ifdef POLYMER
    G_RefreshLights();
#endif
    viewCorrectPrediction();
    ambProcess();
    viewUpdateDelirium();
    viewUpdateShake();
    sfxUpdate3DSounds();
    if (gMe->hand == 1)
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
                gameHandleEvents();
                netMasterUpdate();
            }
        }
        if (gDemo.at0)
            gDemo.Close();
        Mus_Fade(4000);
        seqKillAll();
        if (gGameOptions.uGameFlags&2)
        {
            STAT_Update(true);
            if (gGameOptions.nGameType == 0)
            {
                if (gGameOptions.uGameFlags&8)
                    levelPlayEndScene(gGameOptions.nEpisode);

				M_StartControlPanel(false);
				M_SetMenu(NAME_CreditsMenu);
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


#if 0
SWITCH switches[] = {
    { "broadcast", 1, 0 },
    { "masterslave", 3, 0 },
    { "record", 7, 1 },
    { "robust", 8, 0 },
    { "skill", 10, 1 },
    { "ini", 13, 1 },
    { "noaim", 14, 0 },
    //{ "f", 15, 1 },
    { "control", 16, 1 },
    { "vector", 17, 1 },
    { "noresend", 22, 0 },
    { "silentaim", 23, 0 },
    { "art", 26, 1 },
    { "client", 31, 1 },
    { "noautoload", 32, 0 },
    { NULL, 0, 0 }
};
#endif

void ParseOptions(void)
{
	// Stuff for later.
#if 0
    int option;
    while ((option = GetOptions(switches)) != -1)
    {
        switch (option)
        {
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
            break;
        case 2:
            if (OptArgc < 1)
                ThrowError("Missing argument");
            strcpy(gUserMapFilename, OptArgv[0]);
            bAddUserMap = 1;
            bNoDemo = 1;
            break;
        case 3:
            gPacketMode = PACKETMODE_2;
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
            break;
        }
    }
#endif
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
    //gGameClock++;
}

void ReadAllRFS();

static const char* actions[] = {
    "Move_Forward",
    "Move_Backward",
    "Turn_Left",
    "Turn_Right",
    "Strafe",
    "Fire",
    "Open",
    "Run",
    "Alt_Fire",	// Duke3D", Blood
    "Jump",
    "Crouch",
    "Look_Up",
    "Look_Down",
    "Look_Left",
    "Look_Right",
    "Strafe_Left",
    "Strafe_Right",
    "Aim_Up",
    "Aim_Down",
    "Weapon_1",
    "Weapon_2",
    "Weapon_3",
    "Weapon_4",
    "Weapon_5",
    "Weapon_6",
    "Weapon_7",
    "Weapon_8",
    "Weapon_9",
    "Weapon_10",
    "Inventory",
    "Inventory_Left",
    "Inventory_Right",
    "Nightvision",
    "MedKit",
    "TurnAround",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Center_View",
    "Holster_Weapon",
    "Show_Opponents_Weapon",
    "Map_Follow_Mode",
    "See_Coop_View",
    "Mouse_Aiming",
    "Toggle_Crosshair",
    "Next_Weapon",
    "Previous_Weapon",
    "Dpad_Select",
    "Dpad_Aiming",
    "Third_Person_View",
    "Toggle_Crouch",
    "CrystalBall",
    "ProximityBombs",
    "RemoteBombs",
    "Jetpack"
};

int GameInterface::app_main()
{

    buttonMap.SetButtons(actions, NUM_ACTIONS);
    memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
	gGameOptions.nMonsterSettings = !userConfig.nomonsters;
	bQuickStart = userConfig.nologo;
    ReadAllRFS();
#ifdef USE_QHEAP
    Resource::heap = new QHeap(nMaxAlloc);
#endif

    HookReplaceFunctions();

    Printf("Initializing Build 3D engine\n");
    scrInit();

    Printf("Loading tiles\n");
    if (pUserTiles)
    {
		FStringf buffer("%s%%03i.ART", pUserTiles);
        if (!tileInit(0,buffer))
            ThrowError("User specified ART files not found");
    }
    else
    {
        if (!tileInit(0,NULL))
            ThrowError("TILES###.ART files not found");
    }

    levelLoadDefaults();

    loaddefinitionsfile(BLOODWIDESCREENDEF);
    loaddefinitions_game(BLOODWIDESCREENDEF, FALSE);

    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        Printf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }
    loaddefinitions_game(defsfile, FALSE);
    powerupInit();
    Printf("Loading cosine table\n");
    trigInit();
    Printf("Initializing view subsystem\n");
    viewInit();
    Printf("Initializing dynamic fire\n");
    FireInit();
    Printf("Initializing weapon animations\n");
    WeaponInit();
    LoadSaveSetup();
    LoadSavedInfo();
    gDemo.LoadDemoInfo();
    Printf("There are %d demo(s) in the loop\n", gDemo.at59ef);
    Printf("Loading control setup\n");
    ctrlInit();
    timerInit(120);
    timerSetCallback(ClockStrobe);
    // PORT-TODO: CD audio init

    Printf("Initializing network users\n");
    netInitialize(true);
    scrSetGameMode(0, 0, 0, 0);
    hud_size.Callback();
    Printf("Initializing sound system\n");
    sndInit();
    gChoke.sub_83ff0(518, sub_84230);
    if (bAddUserMap)
    {
        levelAddUserMap(gUserMapFilename);
        gStartNewGame = 1;
    }
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    if (!bQuickStart)
        credLogosDos();

    registerosdcommands();

RESTART:
    sub_79760();
    gViewIndex = myconnectindex;
    gMe = gView = &gPlayer[myconnectindex];
    netBroadcastPlayerInfo(myconnectindex);
    Printf("Waiting for network players!\n");
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
#if 0
    if (!gDemo.at0 && gDemo.at59ef > 0 && gGameOptions.nGameType == 0 && !bNoDemo && demo_playloop)
        gDemo.SetupPlayback(NULL);
#endif
    gQuitGame = 0;
    gRestartGame = 0;
    if (gGameOptions.nGameType > 0)
    {
        inputState.ClearAllInput();
    }
#if 0
    else if (gDemo.at1 && !bAddUserMap && !bNoDemo && demo_playloop)
        gDemo.Playback();
#endif
    if (gDemo.at59ef > 0)
        M_ClearMenus();
	if (!bAddUserMap && !gGameStarted)
	{
		M_StartControlPanel(false);
		M_SetMenu(NAME_Mainmenu);
	}
    ready2send = 1;
    while (!gQuitGame)
    {
        bool bDraw;
        if (gGameStarted)
        {
            char gameUpdate = false;
            double const gameUpdateStartTime = timerGetHiTicks();
            while (gPredictTail < gNetFifoHead[myconnectindex] && !paused)
            {
                viewUpdatePrediction(&gFifoInput[gPredictTail&255][myconnectindex]);
            }
            if (numplayers == 1)
                gBufferJitter = 0;
            while (totalclock >= gNetFifoClock && ready2send)
            {
                gNetInput = gInput;
                gInput = {};
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
                    gameUpdate = true;
                }
            }
            if (gameUpdate)
            {
                g_gameUpdateTime = timerGetHiTicks() - gameUpdateStartTime;
                if (g_gameUpdateAvgTime < 0.f)
                    g_gameUpdateAvgTime = g_gameUpdateTime;
                g_gameUpdateAvgTime = ((GAMEUPDATEAVGTIMENUMSAMPLES-1.f)*g_gameUpdateAvgTime+g_gameUpdateTime)/((float) GAMEUPDATEAVGTIMENUMSAMPLES);
            }
            bDraw = G_FPSLimit() != 0;
            if (gQuitRequest && gQuitGame)
                videoClearScreen(0);
            else
            {
                netCheckSync();
                if (bDraw)
                {
                    viewDrawScreen();
                    g_gameUpdateAndDrawTime = g_beforeSwapTime/* timerGetHiTicks()*/ - gameUpdateStartTime;
                }
            }
        }
        else
        {
            bDraw = G_FPSLimit() != 0;
            if (bDraw)
            {
                twod->ClearScreen();
                rotatesprite(160<<16,100<<16,65536,0,2518,0,0,0x4a,0,0,xdim-1,ydim-1);
            }
            if (gQuitRequest && !gQuitGame)
                netBroadcastMyLogoff(gQuitRequest == 2);
        }
        if (bDraw)
        {
            gameHandleEvents();
            inputState.SetBindsEnabled(gInputMode == kInputGame);
            switch (gInputMode)
            {
            case kInputGame:
                LocalKeys();
                break;
            default:
                break;
            }
            if (gQuitGame)
                continue;

            OSD_DispatchQueued();

            ctrlGetInput();

            switch (gInputMode)
            {
            case kInputMessage:
                gPlayerMsg.ProcessKeys();
                gPlayerMsg.Draw();
                break;
            case kInputEndGame:
                gEndGameMgr.ProcessKeys();
                gEndGameMgr.Draw();
                break;
            default:
                break;
            }
            videoNextPage();
        }
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
		{
			StartLevel(&gGameOptions);
		}
    }
    ready2send = 0;
    if (gDemo.at0)
        gDemo.Close();
    if (gRestartGame)
    {
        UpdateDacs(0, true);
        Mus_Stop();
        soundEngine->StopAllChannels();
        gQuitGame = 0;
        gQuitRequest = 0;
        gRestartGame = 0;
        gGameStarted = 0;
        levelSetupOptions(0,0);
#if 0
		// What's this loop for? Needs checking
        while (gGameMenuMgr.m_bActive)
        {
            gGameMenuMgr.Process();
            if (G_FPSLimit())
            {
                gameHandleEvents();
                videoClearScreen(0);
                gGameMenuMgr.Draw();
                videoNextPage();
            }
        }
#endif
        if (gGameOptions.nGameType != 0)
        {
#if 0
            if (!gDemo.at0 && gDemo.at59ef > 0 && gGameOptions.nGameType == 0 && !bNoDemo && demo_playloop)
                gDemo.NextDemo();
#endif
            videoSetViewableArea(0,0,xdim-1,ydim-1);
            if (!bQuickStart)
                credLogosDos();
        }
        goto RESTART;
    }
    ShutDown();

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

    quoteMgr.InitializeQuote(sel, name);
    return 0;
}

static int parsedefinitions_game(scriptfile *, int);

static void parsedefinitions_game_include(const char *fileName, scriptfile *pScript, const char *cmdtokptr, int const firstPass)
{
    scriptfile *included = scriptfile_fromfile(fileName);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || pScript == NULL) // this is a bit overboard to prevent unused parameter warnings
            {
           // Printf("Warning: Failed including %s as module\n", fn);
            }
/*
        else
            {
            Printf("Warning: Failed including %s on line %s:%d\n",
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
        { "tilefromtexture", T_TILEFROMTEXTURE  },
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

            if (!scriptfile_getstring(pScript,&fileName) && firstPass)
            {
				fileSystem.AddAdditionalFile(fileName);
            }
        }
        break;
        case T_CACHESIZE:
        {
            int32_t cacheSize;

            if (scriptfile_getnumber(pScript, &cacheSize) || !firstPass)
                break;
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
                gNoAutoLoad = true;
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
                    Printf("Error: missing ID for music definition near line %s:%d\n",
                               pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || fileSystem.FileExists(fileName))
                    break;

                if (S_DefineMusic(musicID, fileName) == -1)
                    Printf("Error: invalid music ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
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
				FStringf name("%s.%s", resName, resType);
				fileSystem.CreatePathlessCopy(resName, resID, 0);
            }
        }
        break;

        case T_TILEFROMTEXTURE:
        {
            char *texturetokptr = pScript->ltextptr, *textureend;
            int32_t tile = -1;
            int32_t havesurface = 0, havevox = 0, haveview = 0, haveshade = 0;
            int32_t surface = 0, vox = 0, view = 0, shade = 0;
            int32_t tile_crc32 = 0;
            vec2_t  tile_size{};
            uint8_t have_crc32 = 0;
            uint8_t have_size = 0;

            static const tokenlist tilefromtexturetokens[] =
            {
                { "surface", T_SURFACE },
                { "voxel",   T_VOXEL },
                { "ifcrc",   T_IFCRC },
                { "view",    T_VIEW },
                { "shade",   T_SHADE },
            };

            if (scriptfile_getsymbol(pScript,&tile)) break;
            if (scriptfile_getbraces(pScript,&textureend)) break;
            while (pScript->textptr < textureend)
            {
                int32_t token = getatoken(pScript,tilefromtexturetokens,ARRAY_SIZE(tilefromtexturetokens));
                switch (token)
                {
                case T_IFCRC:
                    scriptfile_getsymbol(pScript, &tile_crc32);
                    have_crc32 = 1;
                    break;
                case T_IFMATCH:
                {
                    char *ifmatchend;

                    static const tokenlist ifmatchtokens[] =
                    {
                        { "crc32",           T_CRC32 },
                        { "size",            T_SIZE },
                    };

                    if (scriptfile_getbraces(pScript,&ifmatchend)) break;
                    while (pScript->textptr < ifmatchend)
                    {
                        int32_t token = getatoken(pScript,ifmatchtokens,ARRAY_SIZE(ifmatchtokens));
                        switch (token)
                        {
                        case T_CRC32:
                            scriptfile_getsymbol(pScript, &tile_crc32);
                            have_crc32 = 1;
                            break;
                        case T_SIZE:
                            scriptfile_getsymbol(pScript, &tile_size.x);
                            scriptfile_getsymbol(pScript, &tile_size.y);
                            have_size = 1;
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                }
                case T_SURFACE:
                    havesurface = 1;
                    scriptfile_getsymbol(pScript, &surface);
                    break;
                case T_VOXEL:
                    havevox = 1;
                    scriptfile_getsymbol(pScript, &vox);
                    break;
                case T_VIEW:
                    haveview = 1;
                    scriptfile_getsymbol(pScript, &view);
                    break;
                case T_SHADE:
                    haveshade = 1;
                    scriptfile_getsymbol(pScript, &shade);
                    break;
                }
            }

            if (!firstPass)
            {
                if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES))
                {
                    Printf("Error: missing or invalid 'tile number' for texture definition near line %s:%d\n",
                               pScript->filename, scriptfile_getlinum(pScript,texturetokptr));
                    break;
                }

                if (have_crc32)
                {
                    int32_t const orig_crc32 = tileGetCRC32(tile);
                    if (orig_crc32 != tile_crc32)
                    {
                        // Printf("CRC32 of tile %d doesn't match! CRC32: %d, Expected: %d\n", tile, orig_crc32, tile_crc32);
                        break;
                    }
                }

                if (have_size)
                {
                    vec2_16_t const orig_size = tilesiz[tile];
                    if (orig_size.x != tile_size.x && orig_size.y != tile_size.y)
                    {
                        // Printf("Size of tile %d doesn't match! Size: (%d, %d), Expected: (%d, %d)\n", tile, orig_size.x, orig_size.y, tile_size.x, tile_size.y);
                        break;
                    }
                }

                if (havesurface)
                    surfType[tile] = surface;
                if (havevox)
                    voxelIndex[tile] = vox;
                if (haveshade)
                    tileShade[tile] = shade;
                if (haveview)
                    picanm[tile].extra = view&7;
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
                Printf("Error: expected animation filename on line %s:%d\n",
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
                    Printf("Error: missing ID for sound definition near line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || fileSystem.FileExists(fileName))
                    break;

                // maybe I should have just packed this into a sound_t and passed a reference...
                if (S_DefineSound(soundNum, fileName, minpitch, maxpitch, priority, type, distance, volume) == -1)
                    Printf("Error: invalid sound ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
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

    if (userConfig.AddDefs) for (auto & m : *userConfig.AddDefs)
        parsedefinitions_game_include(m, NULL, "null", firstPass);

    if (pScript)
        scriptfile_close(pScript);

    scriptfile_clearsymbols();

    return 0;
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

int sndTryPlaySpecialMusic(int nMusic)
{
    if (Mus_Play(nullptr, quoteMgr.GetQuote(nMusic), true))
    {
        return 0;
    }
    return 1;
}

void sndPlaySpecialMusicOrNothing(int nMusic)
{
    if (sndTryPlaySpecialMusic(nMusic))
    {
        Mus_Stop();
    }
}

extern  IniFile* BloodINI;
void GameInterface::FreeGameData()
{
    if (BloodINI) delete BloodINI;
    ShutDown();
}

void GameInterface::UpdateScreenSize()
{
    viewResizeView(gViewSize);
}


::GameInterface* CreateInterface()
{
	return new GameInterface;
}

END_BLD_NS
