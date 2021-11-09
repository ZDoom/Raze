//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "automap.h"
#include "compat.h"
#include "aistuff.h"
#include "player.h"
#include "view.h"
#include "engine.h"
#include "sound.h"
#include "exhumed.h"
#include "input.h"
#include "mapinfo.h"
#include "gamecontrol.h"
#include "v_video.h"
#include "status.h"
#include <stdio.h>
#include <string.h>
#include "statusbar.h"

BEGIN_PS_NS

enum
{
    kTagRamses = 61,
};

int initx, inity, initz;
short inita;
int initsect;

short nCurChunkNum = 0;

DExhumedActor* nBodyGunSprite[50];
int movefifoend;
int movefifopos;

short nCurBodyGunNum;

int SectSoundSect[kMaxSectors] = { 0 };
short SectSound[kMaxSectors]     = { 0 };
short SectFlag[kMaxSectors]      = { 0 };
int   SectDepth[kMaxSectors]     = { 0 };
int   SectAbove[kMaxSectors]     = { 0 };
short SectDamage[kMaxSectors]    = { 0 };
short SectSpeed[kMaxSectors]     = { 0 };
int   SectBelow[kMaxSectors]     = { 0 };

int Counters[kNumCounters];


uint8_t bIsVersion6 = true;




uint8_t LoadLevel(MapRecord* map)
{
    if (map->gameflags & LEVEL_EX_COUNTDOWN)
    {
        lCountDown = 81000;
        nAlarmTicks = 30;
        nRedTicks = 0;
        nClockVal = 0;
        nEnergyTowers = 0;
    }

    initspritelists();


    // init stuff
    {
        StopAllSounds();
        nCreaturesKilled = 0;
        nCreaturesTotal = 0;
        nFreeze = 0;
        pSpiritSprite = nullptr;
        PlayClock = 0;
        memset(Counters, 0, sizeof(Counters));

        InitQueens();
        InitRats();
        InitBullets();
        InitWeapons();
        InitAnims();
        InitSnakes();
        InitLights();
        ClearAutomap();
        InitObjects();
        InitPushBlocks();
		InitPlayer();
        InitItems();

        if (map->gameflags & LEVEL_EX_COUNTDOWN) {
            InitEnergyTile();
        }
    }

    if (map->gameflags & LEVEL_EX_ALTSOUND)
    {
        nSwitchSound = 35;
        nStoneSound = 23;
        nElevSound = 51;
        nStopSound = 35;
    }
    else
    {
        nSwitchSound = 33;
        nStoneSound = 23;
        nElevSound = 23;
        nStopSound = 66;
    }

    vec3_t startPos;
    engineLoadBoard(currentLevel->fileName, 0, &startPos, &inita, &initsect);
    initx = startPos.x;
    inity = startPos.y;
    initz = startPos.z;

    int i;

    for (i = 0; i < kMaxPlayers; i++)
    {
        PlayerList[i].pActor = nullptr;
    }

    psky_t* pSky = tileSetupSky(DEFAULTPSKY);

    pSky->tileofs[0] = 0;
    pSky->tileofs[1] = 0;
    pSky->tileofs[2] = 0;
    pSky->tileofs[3] = 0;
    pSky->yoffs = 256;
    pSky->yoffs2 = 256;
    pSky->lognumtiles = 2;
    pSky->horizfrac = 65536;
    pSky->yscale = 65536;
    parallaxtype = 2;
    g_visibility = 1024;
    flash = 0;
    precache();

    LoadObjects();
    return true;
}

void InitLevel(MapRecord* map)
{
    StopCD();
    currentLevel = map;
    if (!LoadLevel(map)) {
        I_Error("Cannot load %s...\n", map->fileName.GetChars());
    }

    for (int i = 0; i < nTotalPlayers; i++)
    {
        SetSavePoint(i, initx, inity, initz, initsect, inita);
        RestartPlayer(i);
        InitPlayerKeys(i);
    }
    EndLevel = 0;
    lastfps = 0;
    ResetView();
    ResetEngine();
    totalmoves = 0;
    GrabPalette();
    ResetMoveFifo();
    lPlayerXVel = 0;
    lPlayerYVel = 0;
    movefifopos = movefifoend;

    if (!mus_redbook && map->music.IsNotEmpty()) Mus_Play(map->music, true);    // Allow non-CD music if defined for the current level
    playCDtrack(map->cdSongId, true);
	setLevelStarted(currentLevel);
}

void InitNewGame()
{
    bCamera = false;
    PlayerCount = 0;

    for (int i = 0; i < nTotalPlayers; i++)
    {
        int nPlayer = GrabPlayer();
        if (nPlayer < 0) {
            I_Error("Can't create local player\n");
        }

        InitPlayerInventory(nPlayer);
    }
}

void SetBelow(short nCurSector, short nBelowSector)
{
    SectBelow[nCurSector] = nBelowSector;
}

void SetAbove(short nCurSector, short nAboveSector)
{
    SectAbove[nCurSector] = nAboveSector;
}

void SnapSectors(int nSectorA, int nSectorB, int b)
{
    // edx - nSectorA
    // eax - nSectorB

    int nWallA = sector[nSectorA].wallptr;
    int nWallB = sector[nSectorB].wallptr;

    int num1 = sector[nSectorA].wallnum;
    int num2 = sector[nSectorB].wallnum;

    int nCount = 0;

    while (num1 > nCount)
    {
        short dx = nWallB;

        int esi = 0x7FFFFFF;
        int edi = esi;

        int x = wall[nWallA].x;
        int y = wall[nWallA].y;

        int var_14 = 0;

        int nCount2 = 0;

        while (nCount2 < num2)
        {
            int eax = x - wall[dx].x;
            int ebx = y - wall[dx].y;

            if (eax < 0) {
                eax = -eax;
            }

            int var_38 = eax;

            if (ebx < 0) {
                ebx = -ebx;
            }

            int var_3C = ebx;

            var_38 += var_3C;

            eax = esi;
            if (eax < 0) {
                eax = -eax;
            }

            var_3C = eax;

            eax = edi;
//			int var_34 = edi;
            if (eax < 0) {
                eax = -eax;
            }

            int var_34 = eax;

            var_34 += var_3C;

            if (var_38 < var_34)
            {
                esi = x - wall[dx].x;
                edi = y - wall[dx].y;
                var_14 = dx;
            }

            dx++;
            nCount2++;
        }

        dragpoint(var_14, wall[var_14].x + esi, wall[var_14].y + edi, 0);

        nCount++;
        nWallA++;
    }

    if (b) {
        sector[nSectorB].ceilingz = sector[nSectorA].floorz;
    }

    if (SectFlag[nSectorA] & 0x1000) {
        SnapBobs(nSectorA, nSectorB);
    }
}

void InitSectFlag()
{
    for (int i = 0; i < kMaxSectors; i++)
    {
        SectSoundSect[i] = -1;
        SectSound[i] = -1;
        SectAbove[i] = -1;
        SectBelow[i] = -1;
        SectDepth[i] = 0;
        SectFlag[i]  = 0;
        SectSpeed[i] = 0;
        SectDamage[i] = 0;
    }
}

void ProcessSpriteTag(DExhumedActor* pActor, short nLotag, short nHitag)
{
	auto pSprite = &pActor->s();
    int nChannel = runlist_AllocChannel(nHitag % 1000);

    int nSpeed = nLotag / 1000;
    if (!nSpeed) {
        nSpeed = 1;
    }

    int nVal = nHitag;

    if (nLotag >= 900 && nLotag <= 949)
    {
        ProcessTrailSprite(pActor, nLotag, nHitag);
        return;
    }

    // handle tags 6 to 60
    switch (nLotag)
    {
        case 8: // M-60 ammo belt
        {
            nVal = 3 * (nHitag / 3);
            // fall through to 6,7 etc
            fallthrough__;
        }
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 15:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 26:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 60:
        {
            pSprite->hitag = nVal;
            ChangeActorStat(pActor, nLotag + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }
        case 12: // berry twig
        {
            pSprite->hitag = 40;
            ChangeActorStat(pActor, nLotag + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }
        case 13: // blood bowl
        {
            pSprite->hitag = 160;
            ChangeActorStat(pActor, nLotag + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }
        case 14: // venom bowl
        {
            pSprite->hitag = -200;
            ChangeActorStat(pActor, nLotag + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }

        case 16:
            // reserved
            DeleteActor(pActor);
            return;

        case 25:
        case 59:
        {
            // extra life or checkpoint scarab. Delete for multiplayer
            if (nNetPlayerCount != 0)
            {
                DeleteActor(pActor);
                return;
            }
            else
            {
                pSprite->hitag = nVal;
                ChangeActorStat(pActor, nLotag + 900);
                pSprite->cstat &= 0xFEFE;
                BuildItemAnim(pActor);
                return;
            }
        }
        case 27:
        {
            pSprite->hitag = 1;
            ChangeActorStat(pActor, 9 + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }

        case 38: // raw energy
        {
            nVal++;
            nVal--; // CHECKME ??
            pSprite->hitag = nVal;
            ChangeActorStat(pActor, nLotag + 900);
            pSprite->cstat &= 0xFEFE;
            BuildItemAnim(pActor);
            return;
        }
    }

    int v6 = nLotag % 1000;

    if (!userConfig.nomonsters || v6 < 100 || v6 > 118)
    {
        if (v6 > 999) {
            DeleteActor(pActor);
            return;
        }

        switch (v6)
        {
            case 999:
            {
                AddFlicker(pSprite->sectnum, nSpeed);
                break;
            }
            case 998:
            {
                AddGlow(pSprite->sectnum, nSpeed);
                break;
            }
            case 118: // Anubis with drum
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildAnubis(pActor, 0, 0, 0, 0, 0, 1);
                return;
            }
            case 117:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildWasp(pActor, 0, 0, 0, 0, 0, false);
                return;
            }
            case 116:
            {
                BuildRat(pActor, 0, 0, 0, 0, -1);
                return;
            }
            case 115: // Rat (eating)
            {
                BuildRat(pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 113:
            {
                BuildQueen(pActor, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 112:
            {
                BuildScorp(pActor, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 111:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildSet(pActor, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 108:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildLava(pActor, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 107:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildRex(pActor, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 106:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildFish(pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 105:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildSpider(pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 104:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildRoach(1, pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 103:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildRoach(0, pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 102:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildLion(pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 101:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildMummy(pActor, 0, 0, 0, 0, 0);
                return;
            }
            case 100:
            {
                if (userConfig.nomonsters) {
                    DeleteActor(pActor);
                    return;
                }

                BuildAnubis(pActor, 0, 0, 0, 0, 0, 0);
                return;
            }
            case 99: // underwater type 2
            {
                int nSector =pSprite->sectnum;
                SetAbove(nSector, nHitag);
                SectFlag[nSector] |= kSectUnderwater;

                DeleteActor(pActor);
                return;
            }
            case 98:
            {
                int nSector =pSprite->sectnum;
                SetBelow(nSector, nHitag);
                SnapSectors(nSector, nHitag, 1);

                DeleteActor(pActor);
                return;
            }
            case 97:
            {
                AddSectorBob(pSprite->sectnum, nHitag, 1);

                DeleteActor(pActor);
                return;
            }
            case 96: // Lava sector
            {
                int nDamage = nHitag / 4;
                if (!nDamage) {
                    nDamage = 1;
                }

                int nSector =pSprite->sectnum;

                SectDamage[nSector] = nDamage;
                SectFlag[nSector] |= kSectLava;

                DeleteActor(pActor);
                return;
            }
            case 95:
            {
                AddSectorBob(pSprite->sectnum, nHitag, 0);

                DeleteActor(pActor);
                return;
            }
            case 94: // water
            {
                int nSector =pSprite->sectnum;
                SectDepth[nSector] = nHitag << 8;

                DeleteActor(pActor);
                return;
            }
            case 93:
            {
                BuildBubbleMachine(pActor);
                return;
            }
            case 90:
            {
                BuildObject(pActor, 3, nHitag);
                return;
            }
            case 79:
            case 89:
            {
                int nSector =pSprite->sectnum;

                SectSpeed[nSector] = nSpeed;
                SectFlag[nSector] |= pSprite->ang;

                DeleteActor(pActor);
                return;
            }
            case 88:
            {
                AddFlow(pSprite->sectnum, nSpeed, 0, pSprite->ang);

                DeleteActor(pActor);
                return;
            }
            case 80: // underwater
            {
                int nSector =pSprite->sectnum;
                SectFlag[nSector] |= kSectUnderwater;

                DeleteActor(pActor);
                return;
            }
            case 78:
            {
                AddFlow(pSprite->sectnum, nSpeed, 1, pSprite->ang);

                int nSector =pSprite->sectnum;
                SectFlag[nSector] |= 0x8000;

                DeleteActor(pActor);
                return;
            }
            case 77:
            {
                int nArrow = BuildArrow(pActor, nSpeed);

                runlist_AddRunRec(sRunChannels[nChannel].a, nArrow, 0x1F0000);
                return;
            }
            case 76: // Explosion Trigger (Exploding Fire Cauldron)
            {
                BuildObject(pActor, 0, nHitag);
                return;
            }
            case 75: // Explosion Target (Cauldrons, fireballs and grenades will destroy nearby 75 sprites)
            {
                BuildObject(pActor, 1, nHitag);
                return;
            }
            case 71:
            {
                int nFireball = BuildFireBall(pActor, nHitag, nSpeed);

                runlist_AddRunRec(sRunChannels[nChannel].a, nFireball, 0x1F0000);
                return;
            }
            case 70:
            {
                BuildDrip(pActor);
                return;
            }
            case 63:
            {
                ChangeActorStat(pActor, 405);
                pSprite->cstat = 0x8000;
                return;
            }
            case 62:
            {
                nNetStartSprite[nNetStartSprites] = pActor;
                pSprite->cstat = 0x8000;

                nNetStartSprites++;
                return;
            }
            case kTagRamses: // Ramses head
            {
                pSpiritSprite = pActor;
                pSprite->cstat |= 0x8000;
                return;
            }
            default: // TODO - checkme!
            {
                DeleteActor(pActor);
                return;
            }
        }
    }

    DeleteActor(pActor);
}

void ExamineSprites()
{
    nNetStartSprites = 0;
    nCurStartSprite = 0;

	ExhumedLinearSpriteIterator it;
    while (auto ac = it.Next())
    {
		auto pSprite = &ac->s();

        int nStatus = pSprite->statnum;
        if (!nStatus)
        {
            short lotag = pSprite->lotag;
            short hitag = pSprite->hitag;

            if ((nStatus < kMaxStatus) && lotag)
            {
                pSprite->lotag = 0;
                pSprite->hitag = 0;

                ProcessSpriteTag(ac, lotag, hitag);
            }
            else
            {
                ChangeActorStat(ac, 0);
            }
        }
    }

    if (nNetPlayerCount)
    {
        auto pActor = insertActor(initsect, 0);
		auto pSprite = &pActor->s();

        pSprite->x = initx;
        pSprite->y = inity;
        pSprite->z = initz;
        pSprite->cstat = 0x8000;
        nNetStartSprite[nNetStartSprites] = pActor;
        nNetStartSprites++;
    }
}

void LoadObjects()
{
    runlist_InitRun();
    runlist_InitChan();
    InitLink();
    InitPoint();
    InitSlide();
    InitSwitch();
    InitElev();
    InitWallFace();
    InitSectFlag();

    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        auto sectp = &sector[nSector];
        short hitag = sectp->hitag;
        short lotag = sectp->lotag;

        sectp->hitag = 0;
        sectp->lotag = 0;
        sectp->extra = -1;

        if (hitag || lotag)
        {
            sectp->lotag = runlist_HeadRun() + 1;
            sectp->hitag = lotag;

            runlist_ProcessSectorTag(nSector, lotag, hitag);
        }
    }

    for (int nWall = 0; nWall < numwalls; nWall++)
    {
        wall[nWall].extra = -1;

        short lotag = wall[nWall].lotag;
        short hitag = wall[nWall].hitag;

        wall[nWall].lotag = 0;

        if (hitag || lotag)
        {
            wall[nWall].lotag = runlist_HeadRun() + 1;
            runlist_ProcessWallTag(nWall, lotag, hitag);
        }
    }

    ExamineSprites();
    PostProcess();
    InitRa();
    InitChunks();

    for (int nChannel = 0; nChannel < kMaxChannels; nChannel++)
    {
        runlist_ChangeChannel(nChannel, 0);
        runlist_ReadyChannel(nChannel);
    }

    nCamerax = initx;
    nCameray = inity;
    nCameraz = initz;
}

void SerializeInit(FSerializer& arc)
{
    if (arc.BeginObject("init"))
    {
        arc("initx", initx)
            ("inity", inity)
            ("initz", initz)
            ("inita", inita)
            ("initsect", initsect)
            ("curchunk", nCurChunkNum)
            .Array("bodygunsprite", nBodyGunSprite, countof(nBodyGunSprite))
            ("curbodygun", nCurBodyGunNum)
            .Array("soundsect", SectSoundSect, numsectors)
            .Array("sectsound", SectSound, numsectors)
            .Array("sectflag", SectFlag, numsectors)
            .Array("sectdepth", SectDepth, numsectors)
            .Array("sectabove", SectAbove, numsectors)
            .Array("sectdamage", SectDamage, numsectors)
            .Array("sectspeed", SectSpeed, numsectors)
            .Array("sectbelow", SectBelow, numsectors)
            .Array("counters", Counters, kNumCounters)
            .EndObject();
    }
}

END_PS_NS
