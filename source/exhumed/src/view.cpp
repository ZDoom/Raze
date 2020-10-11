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
#include "glbackend/glbackend.h"
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

fixed_t nCameraa;
fixed_t nCamerapan;
short nViewTop;
bool bCamera = false;

int viewz;

short enemy;

short nEnemyPal = 0;

#define MAXINTERPOLATIONS MAXSPRITES
int32_t g_interpolationCnt;
int32_t oldipos[MAXINTERPOLATIONS];
int32_t* curipos[MAXINTERPOLATIONS];
int32_t bakipos[MAXINTERPOLATIONS];

int viewSetInterpolation(int32_t *const posptr)
{
    if (g_interpolationCnt >= MAXINTERPOLATIONS)
        return 1;

    for (int i = 0; i < g_interpolationCnt; ++i)
        if (curipos[i] == posptr)
            return 0;

    curipos[g_interpolationCnt] = posptr;
    oldipos[g_interpolationCnt] = *posptr;
    g_interpolationCnt++;
    return 0;
}

void viewStopInterpolation(const int32_t * const posptr)
{
    for (int i = 0; i < g_interpolationCnt; ++i)
        if (curipos[i] == posptr)
        {
            g_interpolationCnt--;
            oldipos[i] = oldipos[g_interpolationCnt];
            bakipos[i] = bakipos[g_interpolationCnt];
            curipos[i] = curipos[g_interpolationCnt];
        }
}

void viewDoInterpolations(int smoothRatio)
{
    int32_t ndelta = 0;

    for (int i = 0, j = 0; i < g_interpolationCnt; ++i)
    {
        int32_t const odelta = ndelta;
        bakipos[i] = *curipos[i];
        ndelta = (*curipos[i]) - oldipos[i];
        if (odelta != ndelta)
            j = mulscale16(ndelta, smoothRatio);
        *curipos[i] = oldipos[i] + j;
    }
}

void viewUpdateInterpolations(void)  //Stick at beginning of G_DoMoveThings
{
    for (int i=g_interpolationCnt-1; i>=0; i--) oldipos[i] = *curipos[i];
}

void viewRestoreInterpolations(void)  //Stick at end of drawscreen
{
    int32_t i=g_interpolationCnt-1;

    for (; i>=0; i--) *curipos[i] = bakipos[i];
}

// NOTE - not to be confused with Ken's analyzesprites()
static void analyzesprites()
{
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
    tspritetype *pTSprite;

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

                int vcos = Cos(nAngle);
                int vsin = Sin(nAngle);


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
    fixed_t nAngle;
    fixed_t pan;
    fixed_t q16rotscrnang;

    fixed_t dang = IntToFixed(1024);

    zbob = Sin(2 * bobangle) >> 3;

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
        nAngle = IntToFixed(sprite[nSprite].ang);

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

        if (!cl_syncinput)
        {
            nAngle = PlayerList[nLocalPlayer].q16angle + PlayerList[nLocalPlayer].q16look_ang;
            q16rotscrnang = PlayerList[nLocalPlayer].q16rotscrnang;
        }
        else
        {
            fixed_t oang, ang;

            oang = PlayerList[nLocalPlayer].oq16angle + PlayerList[nLocalPlayer].oq16look_ang;
            ang = PlayerList[nLocalPlayer].q16angle + PlayerList[nLocalPlayer].q16look_ang;
            nAngle = oang + xs_CRoundToInt(fmulscale16(((ang + dang - oang) & 0x7FFFFFF) - dang, smoothRatio));

            oang = PlayerList[nLocalPlayer].oq16rotscrnang + PlayerList[nLocalPlayer].oq16rotscrnang;
            ang = PlayerList[nLocalPlayer].q16rotscrnang + PlayerList[nLocalPlayer].q16rotscrnang;
            q16rotscrnang = oang + xs_CRoundToInt(fmulscale16(((ang + dang - oang) & 0x7FFFFFF) - dang, smoothRatio));
        }

        if (!bCamera)
        {
            sprite[nPlayerSprite].cstat |= CSTAT_SPRITE_INVISIBLE;
            sprite[nDoppleSprite[nLocalPlayer]].cstat |= CSTAT_SPRITE_INVISIBLE;
        }

        renderSetRollAngle(FixedToFloat(q16rotscrnang));
    }

    nCameraa = nAngle;

    if (!bCamera || nFreeze || sceneonly)
    {
        if (nSnakeCam >= 0 && !sceneonly)
        {
            pan = IntToFixed(100);
            viewz = playerZ;
        }
        else
        {
            viewz = playerZ + nQuake[nLocalPlayer];
            int floorZ = sector[sprite[nPlayerSprite].sectnum].floorz;

            if (!cl_syncinput)
            {
                pan = PlayerList[nLocalPlayer].q16horiz;
            }
            else
            {
                pan = PlayerList[nLocalPlayer].oq16horiz + xs_CRoundToInt(fmulscale16(PlayerList[nLocalPlayer].q16horiz - PlayerList[nLocalPlayer].oq16horiz, smoothRatio));
            }

            if (viewz > floorZ)
                viewz = floorZ;

            nCameraa += IntToFixed((nQuake[nLocalPlayer] >> 7) % 31);
            nCameraa &= 0x7FFFFFF;
        }
    }
    else
    {
        clipmove_old((int32_t*)&playerX, (int32_t*)&playerY, (int32_t*)&playerZ, &nSector,
            -2000 * Sin(inita + 512),
            -2000 * Sin(inita),
            4, 0, 0, CLIPMASK1);

        pan = IntToFixed(100);
        viewz = playerZ;
    }

    pan = clamp(pan, gi->playerHorizMin(), gi->playerHorizMax());

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
        int const vr = xs_CRoundToInt(65536.f * tanf(r_fov * (fPI / 360.f)));


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

        renderDrawRoomsQ16(nCamerax, nCameray, viewz, nCameraa, nCamerapan, nSector);
        analyzesprites();
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

                    int ang2 = FixedToInt(nCameraa) - sprite[nPlayerSprite].ang;
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


static SavegameHelper sghview("view",
    SV(nCamerax),
    SV(nCameray),
    SV(nCameraz),
    SV(bTouchFloor),
    SV(nChunkTotal),
    SV(nCameraa),
    SV(nCamerapan),
    SV(bCamera),
    SV(viewz),
    SV(enemy),
    SV(nEnemyPal),
    SA(dVertPan),
    SA(nQuake),
    SV(g_interpolationCnt),
    SA(oldipos),
    SA(curipos),
    SA(bakipos),
    nullptr);

END_PS_NS
