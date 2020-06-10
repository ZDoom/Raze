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
#include "snake.h"
#include "gun.h"
#include "light.h"
#include "init.h"
#include "menu.h"
#include "cd.h"
#include "typedefs.h"
#include "map.h"
#include "move.h"
#include "sound.h"
#include "engine.h"
#include "trigdat.h"
#include "runlist.h"
#include "v_video.h"
#include "glbackend/glbackend.h"
#include <string.h>

BEGIN_PS_NS

short bSubTitles = kTrue;

int zbob;

fix16_t nDestVertPan[kMaxPlayers] = { 0 };
short dVertPan[kMaxPlayers];
int nCamerax;
int nCameray;
int nCameraz;

short bTouchFloor;

short nQuake[kMaxPlayers] = { 0 };

short nChunkTotal = 0;

fix16_t nCameraa;
fix16_t nCamerapan;
short nViewTop;
short bClip = kFalse;
short nViewBottom;
short nViewRight;
short besttarget;
short nViewLeft;
short bCamera = kFalse;

short nViewy;

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

    for (bssize_t i = 0; i < g_interpolationCnt; ++i)
        if (curipos[i] == posptr)
            return 0;

    curipos[g_interpolationCnt] = posptr;
    oldipos[g_interpolationCnt] = *posptr;
    g_interpolationCnt++;
    return 0;
}

void viewStopInterpolation(const int32_t * const posptr)
{
    for (bssize_t i = 0; i < g_interpolationCnt; ++i)
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

    for (bssize_t i = 0, j = 0; i < g_interpolationCnt; ++i)
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
    for (bssize_t i=g_interpolationCnt-1; i>=0; i--) oldipos[i] = *curipos[i];
}

void viewRestoreInterpolations(void)  //Stick at end of drawscreen
{
    int32_t i=g_interpolationCnt-1;

    for (; i>=0; i--) *curipos[i] = bakipos[i];
}

void InitView()
{
    screensize = 0;
#ifdef USE_OPENGL
    polymostcenterhoriz = 92;
#endif
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
    //videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, 0);
    DoOverscanSet(overscanindex);
    EraseScreen(overscanindex);
    //videoUpdatePalette(0, 256);
#ifdef USE_OPENGL
    videoTintBlood(0, 0, 0);
#endif

    LoadStatus();
}

void SetView1()
{
}

void RefreshBackground()
{
    if (screensize <= 0)
        return;
    int nTileOffset = 0;
    int tileX = tilesiz[nBackgroundPic].x;
    int tileY = tilesiz[nBackgroundPic].y;

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    MaskStatus();

    for (int y = 0; y < nViewTop; y += tileY)
    {
        nTileOffset = (y/tileY)&1;
        for (int x = 0; x < xdim; x += tileX)
        {
            rotatesprite(x<<16, y<<16, 65536L, 0, nBackgroundPic + nTileOffset, -32, kPalNormal, 8 + 16 + 64, 0, 0, xdim-1, nViewTop-1);
            nTileOffset ^= 1;
        }
    }
    for (int y = (nViewTop/tileY)*tileY; y <= nViewBottom; y += tileY)
    {
        nTileOffset = (y/tileY)&1;
        for (int x = 0; x < nViewLeft; x += tileX)
        {
            rotatesprite(x<<16, y<<16, 65536L, 0, nBackgroundPic + nTileOffset, -32, kPalNormal, 8 + 16 + 64, 0, nViewTop, nViewLeft-1, nViewBottom);
            nTileOffset ^= 1;
        }
    }
    for (int y = (nViewTop/tileY)*tileY; y <= nViewBottom; y += tileY)
    {
        nTileOffset = ((y/tileY)^((nViewRight+1)/tileX))&1;
        for (int x = ((nViewRight+1)/tileX)*tileX; x < xdim; x += tileX)
        {
            rotatesprite(x<<16, y<<16, 65536L, 0, nBackgroundPic + nTileOffset, -32, kPalNormal, 8 + 16 + 64, nViewRight+1, nViewTop, xdim-1, nViewBottom);
            nTileOffset ^= 1;
        }
    }
    for (int y = ((nViewBottom+1)/tileY)*tileY; y < ydim; y += tileY)
    {
        nTileOffset = (y/tileY)&1;
        for (int x = 0; x < xdim; x += tileX)
        {
            rotatesprite(x<<16, y<<16, 65536L, 0, nBackgroundPic + nTileOffset, -32, kPalNormal, 8 + 16 + 64, 0, nViewBottom+1, xdim-1, ydim-1);
            nTileOffset ^= 1;
        }
    }

    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
}

void MySetView(int x1, int y1, int x2, int y2)
{
    if (!bFullScreen) {
        MaskStatus();
    }

    nViewLeft = x1;
    nViewTop = y1;
    nViewRight = x2;
    nViewBottom = y2;

    videoSetViewableArea(x1, y1, x2, y2);

    nViewy = y1;
}

// unused function
void TestLava()
{
}

static inline int interpolate16(int a, int b, int smooth)
{
    return a + mulscale16(b - a, smooth);
}

void DrawView(int smoothRatio, bool sceneonly)
{
    int playerX;
    int playerY;
    int playerZ;
    short nSector;
    fix16_t nAngle;
    fix16_t pan;


    if (!sceneonly)
    {
        RefreshBackground();

        if (!bFullScreen) {
            MaskStatus();
        }
    }

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
        nAngle = fix16_from_int(sprite[nSprite].ang);

        SetGreenPal();
        UnMaskStatus();

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
        nAngle  = PlayerList[nLocalPlayer].q16angle;

        if (!bCamera)
        {
            sprite[nPlayerSprite].cstat |= CSTAT_SPRITE_INVISIBLE;
            sprite[nDoppleSprite[nLocalPlayer]].cstat |= CSTAT_SPRITE_INVISIBLE;
        }
    }

    nCameraa = nAngle;

    if (!bCamera || nFreeze || sceneonly)
    {
        if (nSnakeCam >= 0 && !sceneonly)
        {
            pan = F16(92);
            viewz = playerZ;
        }
        else
        {
            viewz = playerZ + nQuake[nLocalPlayer];
            int floorZ = sector[sprite[nPlayerSprite].sectnum].floorz;

            pan = PlayerList[nLocalPlayer].q16horiz;

            if (viewz > floorZ)
                viewz = floorZ;

            nCameraa += fix16_from_int((nQuake[nLocalPlayer] >> 7) % 31);
            nCameraa &= 0x7FFFFFF;
        }
    }
    else
    {
        clipmove_old((int32_t*)&playerX, (int32_t*)&playerY, (int32_t*)&playerZ, &nSector,
            -2000 * Sin(inita + 512),
            -2000 * Sin(inita),
            4, 0, 0, CLIPMASK1);

        pan = F16(92);
        viewz = playerZ;
    }

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
        UnMaskStatus();
    }

    UpdateMap();

    if (nFreeze != 3)
    {
        static uint8_t sectorFloorPal[MAXSECTORS];
        static uint8_t sectorCeilingPal[MAXSECTORS];
        static uint8_t wallPal[MAXWALLS];
        int const viewingRange = viewingrange;
        int const vr = Blrintf(65536.f * tanf(r_fov * (fPI / 360.f)));

        if (r_usenewaspect)
        {
            newaspect_enable = 1;
            videoSetCorrectedAspect();
            renderSetAspect(mulscale16(vr, viewingrange), yxaspect);
        }
        else
            renderSetAspect(vr, yxaspect);

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

        if (r_usenewaspect)
        {
            newaspect_enable = 0;
            renderSetAspect(viewingRange, tabledivide32_noinline(65536 * ydim * 8, xdim * 5));
        }

        if (nFreeze)
        {
            nSnakeCam = -1;

            if (nFreeze == 2)
            {
                if (nHeadStage == 4)
                {
                    nHeadStage = 5;

                    sprite[nPlayerSprite].cstat |= 0x8000;

                    int ang2 = fix16_to_int(nCameraa) - sprite[nPlayerSprite].ang;
                    if (ang2 < 0)
                        ang2 = -ang2;

                    if (ang2 > 10)
                    {
                        inita -= (ang2 >> 3);
                        return;
                    }

                    if (bSubTitles)
                    {
                        if (levelnum == 1)
                            ReadyCinemaText(1);
                        else
                            ReadyCinemaText(5);
                    }
                }
                else
                {
                    if ((bSubTitles && !AdvanceCinemaText()) || inputState.CheckAllInput())
                    {
						inputState.ClearAllInput();
                        levelnew = levelnum + 1;

                        if (CDplaying()) {
                            fadecdaudio();
                        }
                    }

                    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
                }
            }
        }
        else if (!sceneonly)
        {
            if (nSnakeCam < 0)
            {
                DrawWeapons(smoothRatio);
                DrawMap();
                DrawStatus();
            }
            else
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    sprite[enemy].pal = nEnemyPal;
                }

                DrawMap();

                if (!bFullScreen) {
                    MaskStatus();
                }

                DrawSnakeCamStatus();
            }
        }
    }
    else
    {
        twod->ClearScreen();
        DrawStatus();
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
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    bClip = kFalse;
}

void Clip()
{
    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
    if (!bFullScreen) {
        MaskStatus();
    }

    bClip = kTrue;
}


static SavegameHelper sgh("view",
    SV(nCamerax),
    SV(nCameray),
    SV(nCameraz),
    SV(bTouchFloor),
    SV(nChunkTotal),
    SV(nCameraa),
    SV(nCamerapan),
    SV(nViewTop),
    SV(bClip),
    SV(nViewBottom),
    SV(nViewRight),
    SV(besttarget),
    SV(nViewLeft),
    SV(bCamera),
    SV(nViewy),
    SV(viewz),
    SV(enemy),
    SV(nEnemyPal),
    SA(nDestVertPan),
    SA(dVertPan),
    SA(nQuake),
    SV(g_interpolationCnt),
    SA(oldipos),
    SA(curipos),
    SA(bakipos),
    nullptr);

END_PS_NS
