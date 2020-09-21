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
#include "ps_input.h"
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
short inita, initsect;

short nCurChunkNum = 0;

short nBodyGunSprite[50];
int movefifoend;
int movefifopos;

short nCurBodyGunNum;

short SectSoundSect[kMaxSectors] = { 0 };
short SectSound[kMaxSectors]     = { 0 };
short SectFlag[kMaxSectors]      = { 0 };
int   SectDepth[kMaxSectors]     = { 0 };
int   SectAbove[kMaxSectors]     = { 0 };
short SectDamage[kMaxSectors]    = { 0 };
short SectSpeed[kMaxSectors]     = { 0 };
int   SectBelow[kMaxSectors]     = { 0 };


uint8_t bIsVersion6 = true;




uint8_t LoadLevel(int nMap)
{
    if (nMap == kMap20)
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
        nSpiritSprite = -1;
        leveltime = 0;

        InitLion();
        InitRexs();
        InitSets();
        InitQueens();
        InitRoachs();
        InitWasps();
        InitRats();
        InitBullets();
        InitWeapons();
        InitGrenades();
        InitAnims();
        InitSnakes();
        InitFishes();
        InitLights();
        ClearAutomap();
        InitBubbles();
        InitObjects();
        InitLava();
        InitPushBlocks();
        InitAnubis();
        InitSpider();
        InitMummy();
        InitScorp();
        InitPlayer();
        InitItems();
        InitInput();

        if (nMap == kMap20) {
            InitEnergyTile();
        }
    }

    if (nMap > 15)
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

    if (nMap < 0) {
        return false;
    }

    vec3_t startPos;
    int status = engineLoadBoard(currentLevel->fileName, 0, &startPos, &inita, &initsect);
    if (status == -2)
        status = engineLoadBoardV5V6(currentLevel->fileName, 0, &startPos, &inita, &initsect);
    initx = startPos.x;
    inity = startPos.y;
    initz = startPos.z;

    int i;

    for (i = 0; i < kMaxPlayers; i++)
    {
        PlayerList[i].nSprite = -1;
    }

    psky_t* pSky = tileSetupSky(DEFAULTPSKY);

    pSky->tileofs[0] = 0;
    pSky->tileofs[1] = 0;
    pSky->tileofs[2] = 0;
    pSky->tileofs[3] = 0;
    pSky->yoffs = 256;
    pSky->lognumtiles = 2;
    pSky->horizfrac = 65536;
    pSky->yscale = 65536;
    parallaxtype = 2;
    g_visibility = 2048;
    flash = 0;
    precache();

    LoadObjects();
    return true;
}

void InitLevel(int level) // todo: use a map record
{
    StopCD();
    currentLevel = FindMapByLevelNum(level);
    if (!LoadLevel(level)) {
        I_Error("Can't load level %d...\n", level);
    }

    for (int i = 0; i < nTotalPlayers; i++)
    {
        SetSavePoint(i, initx, inity, initz, initsect, inita);
        RestartPlayer(i);
        InitPlayerKeys(i);
    }
    EndLevel = 0;
    lastfps = 0;
    InitStatus();
    ResetView();
    ResetEngine();
    totalmoves = 0;
    GrabPalette();
    ResetMoveFifo();
    lPlayerXVel = 0;
    lPlayerYVel = 0;
    movefifopos = movefifoend;

    RefreshStatus();

    int nTrack = level;
    if (nTrack != 0) nTrack--;

    playCDtrack((nTrack % 8) + 11, true);
	setLevelStarted(currentLevel);
}

void InitNewGame()
{
    bCamera = false;
    nCinemaSeen = 0;
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

void SnapSectors(short nSectorA, short nSectorB, short b)
{
    // edx - nSectorA
    // eax - nSectorB

    short nWallA = sector[nSectorA].wallptr;
    short nWallB = sector[nSectorB].wallptr;

    short num1 = sector[nSectorA].wallnum;
    short num2 = sector[nSectorB].wallnum;

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

void ProcessSpriteTag(short nSprite, short nLotag, short nHitag)
{
    int nChannel = runlist_AllocChannel(nHitag % 1000);

    int nSpeed = nLotag / 1000;
    if (!nSpeed) {
        nSpeed = 1;
    }

    int nVal = nHitag;

    if (nLotag >= 900 && nLotag <= 949)
    {
        ProcessTrailSprite(nSprite, nLotag, nHitag);
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
            sprite[nSprite].hitag = nVal;
            changespritestat(nSprite, nLotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 12: // berry twig
        {
            sprite[nSprite].hitag = 40;
            changespritestat(nSprite, nLotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 13: // blood bowl
        {
            sprite[nSprite].hitag = 160;
            changespritestat(nSprite, nLotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 14: // venom bowl
        {
            sprite[nSprite].hitag = -200;
            changespritestat(nSprite, nLotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }

        case 16:
            // reserved
            mydeletesprite(nSprite);
            return;

        case 25:
        case 59:
        {
            // extra life or checkpoint scarab. Delete for multiplayer
            if (nNetPlayerCount != 0)
            {
                mydeletesprite(nSprite);
                return;
            }
            else
            {
                sprite[nSprite].hitag = nVal;
                changespritestat(nSprite, nLotag + 900);
                sprite[nSprite].cstat &= 0xFEFE;
                BuildItemAnim(nSprite);
                return;
            }
        }
        case 27:
        {
            sprite[nSprite].hitag = 1;
            changespritestat(nSprite, 9 + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }

        case 38: // raw energy
        {
            nVal++;
            nVal--; // CHECKME ??
            sprite[nSprite].hitag = nVal;
            changespritestat(nSprite, nLotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
    }

    int v6 = nLotag % 1000;

    if (!userConfig.nomonsters || v6 < 100 || v6 > 118)
    {
        if (v6 > 999) {
            mydeletesprite(nSprite);
            return;
        }

        switch (v6)
        {
            case 999:
            {
                AddFlicker(sprite[nSprite].sectnum, nSpeed);
                break;
            }
            case 998:
            {
                AddGlow(sprite[nSprite].sectnum, nSpeed);
                break;
            }
            case 118: // Anubis with drum
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildAnubis(nSprite, 0, 0, 0, 0, 0, 1);
                return;
            }
            case 117:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildWasp(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 116:
            {
                BuildRat(nSprite, 0, 0, 0, 0, -1);
                return;
            }
            case 115: // Rat (eating)
            {
                BuildRat(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 113:
            {
                BuildQueen(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 112:
            {
                BuildScorp(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 111:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildSet(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 108:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildLava(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 107:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRex(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 106:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildFish(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 105:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildSpider(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 104:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRoach(1, nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 103:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRoach(0, nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 102:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildLion(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 101:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildMummy(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 100:
            {
                if (userConfig.nomonsters) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildAnubis(nSprite, 0, 0, 0, 0, 0, 0);
                return;
            }
            case 99: // underwater type 2
            {
                short nSector = sprite[nSprite].sectnum;
                SetAbove(nSector, nHitag);
                SectFlag[nSector] |= kSectUnderwater;

                mydeletesprite(nSprite);
                return;
            }
            case 98:
            {
                short nSector = sprite[nSprite].sectnum;
                SetBelow(nSector, nHitag);
                SnapSectors(nSector, nHitag, 1);

                mydeletesprite(nSprite);
                return;
            }
            case 97:
            {
                AddSectorBob(sprite[nSprite].sectnum, nHitag, 1);

                mydeletesprite(nSprite);
                return;
            }
            case 96: // Lava sector
            {
                int nDamage = nHitag / 4;
                if (!nDamage) {
                    nDamage = 1;
                }

                short nSector = sprite[nSprite].sectnum;

                SectDamage[nSector] = nDamage;
                SectFlag[nSector] |= kSectLava;

                mydeletesprite(nSprite);
                return;
            }
            case 95:
            {
                AddSectorBob(sprite[nSprite].sectnum, nHitag, 0);

                mydeletesprite(nSprite);
                return;
            }
            case 94: // water
            {
                short nSector = sprite[nSprite].sectnum;
                SectDepth[nSector] = nHitag << 8;

                mydeletesprite(nSprite);
                return;
            }
            case 93:
            {
                BuildBubbleMachine(nSprite);
                return;
            }
            case 90:
            {
                BuildObject(nSprite, 3, nHitag);
                return;
            }
            case 79:
            case 89:
            {
                short nSector = sprite[nSprite].sectnum;

                SectSpeed[nSector] = nSpeed;
                SectFlag[nSector] |= sprite[nSprite].ang;

                mydeletesprite(nSprite);
                return;
            }
            case 88:
            {
                AddFlow(nSprite, nSpeed, 0);

                mydeletesprite(nSprite);
                return;
            }
            case 80: // underwater
            {
                short nSector = sprite[nSprite].sectnum;
                SectFlag[nSector] |= kSectUnderwater;

                mydeletesprite(nSprite);
                return;
            }
            case 78:
            {
                AddFlow(nSprite, nSpeed, 1);

                short nSector = sprite[nSprite].sectnum;
                SectFlag[nSector] |= 0x8000;

                mydeletesprite(nSprite);
                return;
            }
            case 77:
            {
                int nArrow = BuildArrow(nSprite, nSpeed);

                runlist_AddRunRec(sRunChannels[nChannel].a, nArrow);
                return;
            }
            case 76: // Explosion Trigger (Exploding Fire Cauldron)
            {
                BuildObject(nSprite, 0, nHitag);
                return;
            }
            case 75: // Explosion Target (Cauldrons, fireballs and grenades will destroy nearby 75 sprites)
            {
                BuildObject(nSprite, 1, nHitag);
                return;
            }
            case 71:
            {
                int nFireball = BuildFireBall(nSprite, nHitag, nSpeed);

                runlist_AddRunRec(sRunChannels[nChannel].a, nFireball);
                return;
            }
            case 70:
            {
                BuildDrip(nSprite);
                return;
            }
            case 63:
            {
                changespritestat(nSprite, 405);
                sprite[nSprite].cstat = 0x8000;
                return;
            }
            case 62:
            {
                nNetStartSprite[nNetStartSprites] = nSprite;
                sprite[nSprite].cstat = 0x8000;

                nNetStartSprites++;
                return;
            }
            case kTagRamses: // Ramses head
            {
                nSpiritSprite = nSprite;
                sprite[nSprite].cstat |= 0x8000;
                return;
            }
            default: // TODO - checkme!
            {
                mydeletesprite(nSprite);
                return;
            }
        }
    }

    mydeletesprite(nSprite);
}

void ExamineSprites()
{
    nNetStartSprites = 0;
    nCurStartSprite = 0;

    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        int nStatus = sprite[nSprite].statnum;
        if (!nStatus)
        {
            short lotag = sprite[nSprite].lotag;
            short hitag = sprite[nSprite].hitag;

            if ((nStatus < kMaxStatus) && lotag)
            {
                sprite[nSprite].lotag = 0;
                sprite[nSprite].hitag = 0;

                ProcessSpriteTag(nSprite, lotag, hitag);
            }
            else
            {
                changespritestat(nSprite, 0);
            }
        }
    }

    if (nNetPlayerCount)
    {
        int nSprite = insertsprite(initsect, 0);
        sprite[nSprite].x = initx;
        sprite[nSprite].y = inity;
        sprite[nSprite].z = initz;
        sprite[nSprite].cstat = 0x8000;
        nNetStartSprite[nNetStartSprites] = nSprite;
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
        short hitag = sector[nSector].hitag;
        short lotag = sector[nSector].lotag;

        sector[nSector].hitag = 0;
        sector[nSector].lotag = 0;
        sector[nSector].extra = -1;

        if (hitag || lotag)
        {
            sector[nSector].lotag = runlist_HeadRun() + 1;
            sector[nSector].hitag = lotag;

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


static SavegameHelper sgh("init",
    SV(initx),
    SV(inity),
    SV(initz),
    SV(inita),
    SV(initsect),
    SV(nCurChunkNum),
    SA(nBodyGunSprite),
    SV(nCurBodyGunNum),
    SA(SectSoundSect),
    SA(SectSound),
    SA(SectFlag),
    SA(SectDepth),
    SA(SectAbove),
    SA(SectDamage),
    SA(SectSpeed),
    SA(SectBelow),
    nullptr);


END_PS_NS
