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
#include "compat.h"
#include "engine.h"
#include "names.h"
#include "view.h"
#include "status.h"
#include "exhumed.h"
#include "player.h"
#include "aistuff.h"
#include "sound.h"
#include "mapinfo.h"
#include "v_video.h"
#include "interpolate.h"
#include "glbackend/glbackend.h"
#include "v_draw.h"
#include <string.h>

BEGIN_PS_NS

short bSubTitles = true;

int zbob;

short dVertPan[kMaxPlayers];
int nCamerax;
int nCameray;
int nCameraz;


short bTouchFloor;

short nQuake[kMaxPlayers] = { 0 };

short nChunkTotal = 0;

binangle nCameraa;
fixedhoriz nCamerapan;
short nViewTop;
bool bCamera = false;

int viewz;

short enemy;

short nEnemyPal = 0;

// NOTE - not to be confused with Ken's analyzesprites()
static void analyzesprites(double const smoothratio)
{
    tspritetype *pTSprite;

    for (int i = 0; i < spritesortcnt; i++) {
        pTSprite = &tsprite[i];

        if (pTSprite->owner != -1)
        {
            // interpolate sprite position
            Loc* oldLoc = &oldLocs[pTSprite->owner];
            pTSprite->x = oldLoc->x + MulScale(pTSprite->x - oldLoc->x, smoothratio, 16);
            pTSprite->y = oldLoc->y + MulScale(pTSprite->y - oldLoc->y, smoothratio, 16);
            pTSprite->z = oldLoc->z + MulScale(pTSprite->z - oldLoc->z, smoothratio, 16);
            pTSprite->ang = oldLoc->ang + MulScale(((pTSprite->ang - oldLoc->ang + 1024) & 0x7FF) - 1024, smoothratio, 16);
        }
    }

    short nPlayerSprite = PlayerList[nLocalPlayer].nSprite;

    int var_38 = 20;
    int var_2C = 30000;

    spritetype *pPlayerSprite = &sprite[nPlayerSprite];

    besttarget = -1;

    int x = pPlayerSprite->x;
    int y = pPlayerSprite->y;

    int z = pPlayerSprite->z - (GetSpriteHeight(nPlayerSprite) / 2);

    short nSector = pPlayerSprite->sectnum;

    int nAngle = (2048 - pPlayerSprite->ang) & kAngleMask;

    int nTSprite;

//	int var_20 = var_24;

    for (nTSprite = spritesortcnt-1, pTSprite = &tsprite[nTSprite]; nTSprite >= 0; nTSprite--, pTSprite--)
    {
        int nSprite = pTSprite->owner;
        spritetype *pSprite = &sprite[nSprite];

        if (pTSprite->sectnum >= 0)
        {
            sectortype *pSector = &sector[pTSprite->sectnum];
            int nSectShade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
            int nShade = pTSprite->shade + nSectShade + 6;
            pTSprite->shade = clamp(nShade, -128, 127);
        }

        pTSprite->pal = RemapPLU(pTSprite->pal);

        if (pSprite->statnum > 0)
        {
            runlist_SignalRun(pSprite->lotag - 1, nTSprite | 0x90000);

            if ((pSprite->statnum < 150) && (pSprite->cstat & 0x101) && (nSprite != nPlayerSprite))
            {
                int xval = pSprite->x - x;
                int yval = pSprite->y - y;

                int vcos = bcos(nAngle);
                int vsin = bsin(nAngle);


                int edx = ((vcos * yval) + (xval * vsin)) >> 14;


                int ebx = klabs(((vcos * xval) - (yval * vsin)) >> 14);

                if (!ebx)
                    continue;

                edx = (klabs(edx) * 32) / ebx;
                if (ebx < 1000 && ebx < var_2C && edx < 10)
                {
                    besttarget = nSprite;
                    var_38 = edx;
                    var_2C = ebx;
                }
                else if (ebx < 30000)
                {
                    int t = var_38 - edx;
                    if (t > 3 || (ebx < var_2C && klabs(t) < 5))
                    {
                        var_38 = edx;
                        var_2C = ebx;
                        besttarget = nSprite;
                    }
                }
            }
        }
    }
    if (besttarget != -1)
    {
        spritetype *pTarget = &sprite[besttarget];

        nCreepyTimer = kCreepyCount;

        if (!cansee(x, y, z, nSector, pTarget->x, pTarget->y, pTarget->z - GetSpriteHeight(besttarget), pTarget->sectnum))
        {
            besttarget = -1;
        }
    }
}

void ResetView()
{
    EraseScreen(overscanindex);
#ifdef USE_OPENGL
    videoTintBlood(0, 0, 0);
#endif
}

static inline int interpolate16(int a, int b, int smooth)
{
    return a + mulscale16(b - a, smooth);
}

static TextOverlay subtitleOverlay;

void DrawView(double smoothRatio, bool sceneonly)
{
    int playerX;
    int playerY;
    int playerZ;
    short nSector;
    binangle nAngle;
    fixedhoriz pan;
    lookangle rotscrnang;

    fixed_t dang = IntToFixed(1024);

    zbob = bsin(2 * bobangle, -3);

    DoInterpolations(smoothRatio / 65536.);

    int nPlayerSprite = PlayerList[nLocalPlayer].nSprite;
    int nPlayerOldCstat = sprite[nPlayerSprite].cstat;
    int nDoppleOldCstat = sprite[nDoppleSprite[nLocalPlayer]].cstat;

    if (nSnakeCam >= 0 && !sceneonly)
    {
        int nSprite = SnakeList[nSnakeCam].nSprites[0];

        playerX = sprite[nSprite].x;
        playerY = sprite[nSprite].y;
        playerZ = sprite[nSprite].z;
        nSector = sprite[nSprite].sectnum;
        nAngle = buildang(sprite[nSprite].ang);

        SetGreenPal();

        enemy = SnakeList[nSnakeCam].nEnemy;

        if (enemy <= -1 || totalmoves & 1)
        {
            nEnemyPal = -1;
        }
        else
        {
            nEnemyPal = sprite[enemy].pal;
            sprite[enemy].pal = 5;
        }
    }
    else
    {
        playerX = interpolate16(PlayerList[nLocalPlayer].opos.x, sprite[nPlayerSprite].x, smoothRatio);
        playerY = interpolate16(PlayerList[nLocalPlayer].opos.y, sprite[nPlayerSprite].y, smoothRatio);
        playerZ = interpolate16(PlayerList[nLocalPlayer].opos.z, sprite[nPlayerSprite].z, smoothRatio)
                + interpolate16(oeyelevel[nLocalPlayer], eyelevel[nLocalPlayer], smoothRatio);
        nSector = nPlayerViewSect[nLocalPlayer];
        updatesector(playerX, playerY, &nSector);

        if (!SyncInput())
        {
            nAngle = PlayerList[nLocalPlayer].angle.sum();
            rotscrnang = PlayerList[nLocalPlayer].angle.rotscrnang;
        }
        else
        {
            nAngle = PlayerList[nLocalPlayer].angle.interpolatedsum(smoothRatio);
            rotscrnang = PlayerList[nLocalPlayer].angle.interpolatedrotscrn(smoothRatio);
        }

        if (!bCamera)
        {
            sprite[nPlayerSprite].cstat |= CSTAT_SPRITE_INVISIBLE;
            sprite[nDoppleSprite[nLocalPlayer]].cstat |= CSTAT_SPRITE_INVISIBLE;
        }

        renderSetRollAngle(rotscrnang.asbuildf());
    }

    nCameraa = nAngle;

    if (!bCamera || nFreeze || sceneonly)
    {
        if (nSnakeCam >= 0 && !sceneonly)
        {
            pan = q16horiz(0);
            viewz = playerZ;
        }
        else
        {
            viewz = playerZ + nQuake[nLocalPlayer];
            int floorZ = sector[sprite[nPlayerSprite].sectnum].floorz;

            if (!SyncInput())
            {
                pan = PlayerList[nLocalPlayer].horizon.sum();
            }
            else
            {
                pan = PlayerList[nLocalPlayer].horizon.interpolatedsum(smoothRatio);
            }

            if (viewz > floorZ)
                viewz = floorZ;

            nCameraa += buildang((nQuake[nLocalPlayer] >> 7) % 31);
        }
    }
    else
    {
        clipmove_old((int32_t*)&playerX, (int32_t*)&playerY, (int32_t*)&playerZ, &nSector,
            -2000 * bcos(inita),
            -2000 * bsin(inita),
            4, 0, 0, CLIPMASK1);

        pan = q16horiz(0);
        viewz = playerZ;
    }

    pan = q16horiz(clamp(pan.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));

    nCamerax = playerX;
    nCameray = playerY;
    nCameraz = playerZ;

    int Z = sector[nSector].ceilingz + 256;
    if (Z <= viewz)
    {
        Z = sector[nSector].floorz - 256;

        if (Z < viewz)
            viewz = Z;
    }
    else {
        viewz = Z;
    }

    nCamerapan = pan;

    if (nFreeze == 2 || nFreeze == 1)
    {
        nSnakeCam = -1;
        videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    }

    UpdateMap();

    if (nFreeze != 3)
    {
        static uint8_t sectorFloorPal[MAXSECTORS];
        static uint8_t sectorCeilingPal[MAXSECTORS];
        static uint8_t wallPal[MAXWALLS];
        int const viewingRange = viewingrange;
        int const vr = xs_CRoundToInt(65536.f * tanf(r_fov * (pi::pi() / 360.f)));


        videoSetCorrectedAspect();
        renderSetAspect(mulscale16(vr, viewingrange), yxaspect);

        if (HavePLURemap())
        {
            for (int i = 0; i < numsectors; i++)
            {
                sectorFloorPal[i] = sector[i].floorpal;
                sectorCeilingPal[i] = sector[i].ceilingpal;
                sector[i].floorpal = RemapPLU(sectorFloorPal[i]);
                sector[i].ceilingpal = RemapPLU(sectorCeilingPal[i]);
            }
            for (int i = 0; i < numwalls; i++)
            {
                wallPal[i] = wall[i].pal;
                wall[i].pal = RemapPLU(wallPal[i]);
            }
        }

        renderDrawRoomsQ16(nCamerax, nCameray, viewz, nCameraa.asq16(), nCamerapan.asq16(), nSector);
        analyzesprites(smoothRatio);
        renderDrawMasks();

        if (HavePLURemap())
        {
            for (int i = 0; i < numsectors; i++)
            {
                sector[i].floorpal = sectorFloorPal[i];
                sector[i].ceilingpal = sectorCeilingPal[i];
            }
            for (int i = 0; i < numwalls; i++)
            {
                wall[i].pal = wallPal[i];
            }
        }


        renderSetAspect(viewingRange, divscale16(ydim * 8, xdim * 5));

        if (nFreeze)
        {
            nSnakeCam = -1;

            if (nFreeze == 2)
            {
                if (nHeadStage == 4)
                {
                    nHeadStage = 5;

                    sprite[nPlayerSprite].cstat |= 0x8000;

                    int ang2 = nCameraa.asbuild() - sprite[nPlayerSprite].ang;
                    if (ang2 < 0)
                        ang2 = -ang2;

                    if (ang2 > 10)
                    {
                        inita -= (ang2 >> 3);
                        return;
                    }

                    if (bSubTitles)
                    {
                        subtitleOverlay.Start(I_GetTimeNS() * (120. / 1'000'000'000));
                        if (currentLevel->levelNumber == 1)
                            subtitleOverlay.ReadyCinemaText(1);
                        else
                            subtitleOverlay.ReadyCinemaText(5);
                    }
                    inputState.ClearAllInput();
                }
                else if (nHeadStage == 5)
                {
                    if ((bSubTitles && !subtitleOverlay.AdvanceCinemaText(I_GetTimeNS() * (120. / 1'000'000'000))) || inputState.CheckAllInput())
                    {
                        inputState.ClearAllInput();
                        LevelFinished();
                        EndLevel = 1;

                        if (CDplaying()) {
                            fadecdaudio();
                        }
                    }
                    else subtitleOverlay.DisplayText();
                }
            }
        }
        else if (!sceneonly)
        {
            if (nSnakeCam < 0)
            {
                DrawWeapons(smoothRatio);
                DrawMap();
            }
            else
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    sprite[enemy].pal = nEnemyPal;
                }

                DrawMap();
            }
        }
    }
    else
    {
        twod->ClearScreen();
    }

    sprite[nPlayerSprite].cstat = nPlayerOldCstat;
    sprite[nDoppleSprite[nLocalPlayer]].cstat = nDoppleOldCstat;
    RestoreInterpolations();

    flash = 0;
}

bool GameInterface::GenerateSavePic()
{
    DrawView(65536, true);
    return true;
}

void NoClip()
{
}

void Clip()
{
}

void SerializeView(FSerializer& arc)
{
    arc("camerax", nCamerax)
        ("cameray", nCameray)
        ("cameraz", nCameraz)
        ("touchfloor", bTouchFloor)
        ("chunktotal", nChunkTotal)
        ("cameraa", nCameraa)
        ("camerapan", nCamerapan)
        ("camera", bCamera)
        ("viewz", viewz)
        ("enemy", enemy)
        ("enemypal", nEnemyPal)
        .Array("vertpan", dVertPan, countof(dVertPan))
        .Array("quake", nQuake, countof(nQuake))
        .EndObject();
}

END_PS_NS
