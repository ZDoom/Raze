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
#include "gamefuncs.h"
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
#include "v_draw.h"
#include "render.h"
#include <string.h>

EXTERN_CVAR(Bool, testnewrenderer)

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

DExhumedActor* pEnemy;

short nEnemyPal = 0;

// We cannot drag these through the entire event system... :(
spritetype* mytsprite;
int* myspritesortcnt;

// NOTE - not to be confused with Ken's analyzesprites()
static void analyzesprites(spritetype* tsprite, int& spritesortcnt, int x, int y, int z, double const smoothratio)
{
    tspritetype *pTSprite;

    mytsprite = tsprite;
    myspritesortcnt = &spritesortcnt;

    for (int i = 0; i < spritesortcnt; i++) {
        pTSprite = &tsprite[i];

        if (pTSprite->owner != -1)
        {
            // interpolate sprite position
            pTSprite->pos = pTSprite->interpolatedvec3(smoothratio);
            pTSprite->ang = pTSprite->interpolatedang(smoothratio);
        }
    }

    auto pPlayerActor = PlayerList[nLocalPlayer].Actor();

    int var_38 = 20;
    int var_2C = 30000;

    spritetype *pPlayerSprite = &pPlayerActor->s();

    bestTarget = nullptr;

    int nSector =pPlayerSprite->sectnum;

    int nAngle = (2048 - pPlayerSprite->ang) & kAngleMask;

    int nTSprite;

//	int var_20 = var_24;

    for (nTSprite = spritesortcnt-1, pTSprite = &tsprite[nTSprite]; nTSprite >= 0; nTSprite--, pTSprite--)
    {
        int nSprite = pTSprite->owner;
        auto pActor = &exhumedActors[nSprite];
        spritetype *pSprite = &pActor->s();

        if (pTSprite->sectnum >= 0)
        {
            sectortype *pSector = &sector[pTSprite->sectnum];
            int nSectShade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
            int nShade = pTSprite->shade + nSectShade + 6;
            pTSprite->shade = clamp(nShade, -128, 127);
        }

        pTSprite->pal = RemapPLU(pTSprite->pal);

        // PowerSlaveGDX: Torch bouncing fix
        if ((pTSprite->picnum == kTorch1 || pTSprite->picnum == kTorch2) && (pTSprite->cstat & 0x80) == 0)
        {
            pTSprite->cstat |= 0x80;
            int nTileY = (tileHeight(pTSprite->picnum) * pTSprite->yrepeat) * 2;
            pTSprite->z -= nTileY;
        }

        if (pSprite->statnum > 0)
        {
            RunListEvent ev{};
            ev.pTSprite = pTSprite;
            runlist_SignalRun(pSprite->lotag - 1, nTSprite, &ExhumedAI::Draw, &ev);

            if ((pSprite->statnum < 150) && (pSprite->cstat & 0x101) && (pActor != pPlayerActor))
            {
                int xval = pSprite->x - x;
                int yval = pSprite->y - y;

                int vcos = bcos(nAngle);
                int vsin = bsin(nAngle);


                int edx = ((vcos * yval) + (xval * vsin)) >> 14;


                int ebx = abs(((vcos * xval) - (yval * vsin)) >> 14);

                if (!ebx)
                    continue;

                edx = (abs(edx) * 32) / ebx;
                if (ebx < 1000 && ebx < var_2C && edx < 10)
                {
                    bestTarget = pActor;
                    var_38 = edx;
                    var_2C = ebx;
                }
                else if (ebx < 30000)
                {
                    int t = var_38 - edx;
                    if (t > 3 || (ebx < var_2C && abs(t) < 5))
                    {
                        var_38 = edx;
                        var_2C = ebx;
                        bestTarget = pActor;
                    }
                }
            }
        }
    }
    if (bestTarget != nullptr)
    {
        spritetype *pTarget = &bestTarget->s();

        nCreepyTimer = kCreepyCount;

        if (!cansee(x, y, z, nSector, pTarget->x, pTarget->y, pTarget->z - GetActorHeight(bestTarget), pTarget->sectnum))
        {
            bestTarget = nullptr;
        }
    }

    mytsprite = nullptr;
    myspritesortcnt = nullptr;

}

void ResetView()
{
    EraseScreen(overscanindex);
#ifdef USE_OPENGL
    videoTintBlood(0, 0, 0);
#endif
}

static TextOverlay subtitleOverlay;

void DrawView(double smoothRatio, bool sceneonly)
{
    int playerX;
    int playerY;
    int playerZ;
    int nSector;
    binangle nAngle, rotscrnang;
    fixedhoriz pan;

    fixed_t dang = IntToFixed(1024);

    zbob = bsin(2 * bobangle, -3);

    DoInterpolations(smoothRatio / 65536.);
    pm_smoothratio = (int)smoothRatio;

    auto pPlayerActor = PlayerList[nLocalPlayer].Actor();
	auto pPlayerSprite = &pPlayerActor->s();
    int nPlayerOldCstat = pPlayerSprite->cstat;
    auto pDop = &PlayerList[nLocalPlayer].pDoppleSprite->s();
    int nDoppleOldCstat = pDop->cstat;

    if (nSnakeCam >= 0 && !sceneonly)
    {
        auto pActor = SnakeList[nSnakeCam].pSprites[0];
		auto pSprite = &pActor->s();

        playerX = pSprite->x;
        playerY = pSprite->y;
        playerZ = pSprite->z;
        nSector = pSprite->sectnum;
        nAngle = buildang(pSprite->ang);
        rotscrnang = buildang(0);

        SetGreenPal();

        pEnemy = SnakeList[nSnakeCam].pEnemy;

        if (pEnemy == nullptr || totalmoves & 1)
        {
            nEnemyPal = -1;
        }
        else
        {
            nEnemyPal = pEnemy->s().pal;
            pEnemy->s().pal = 5;
        }
    }
    else
    {
        playerX = pPlayerSprite->interpolatedx(smoothRatio);
        playerY = pPlayerSprite->interpolatedy(smoothRatio);
        playerZ = pPlayerSprite->interpolatedz(smoothRatio) + interpolatedvalue(PlayerList[nLocalPlayer].oeyelevel, PlayerList[nLocalPlayer].eyelevel, smoothRatio);

        nSector = PlayerList[nLocalPlayer].nPlayerViewSect;
        updatesector(playerX, playerY, &nSector);

        if (!SyncInput())
        {
            pan = PlayerList[nLocalPlayer].horizon.sum();
            nAngle = PlayerList[nLocalPlayer].angle.sum();
            rotscrnang = PlayerList[nLocalPlayer].angle.rotscrnang;
        }
        else
        {
            pan = PlayerList[nLocalPlayer].horizon.interpolatedsum(smoothRatio);
            nAngle = PlayerList[nLocalPlayer].angle.interpolatedsum(smoothRatio);
            rotscrnang = PlayerList[nLocalPlayer].angle.interpolatedrotscrn(smoothRatio);
        }

        if (!bCamera)
        {
            pPlayerSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            pDop->cstat |= CSTAT_SPRITE_INVISIBLE;
        }
        else
        {
            pPlayerSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
            pDop->cstat |= CSTAT_SPRITE_INVISIBLE;
        }
        pan = q16horiz(clamp(pan.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));
    }

    nCameraa = nAngle;

    if (nSnakeCam >= 0 && !sceneonly)
    {
        pan = q16horiz(0);
        viewz = playerZ;
    }
    else
    {
        viewz = playerZ + nQuake[nLocalPlayer];
        int floorZ = pPlayerSprite->sector()->floorz;

        if (viewz > floorZ)
            viewz = floorZ;

        nCameraa += buildang((nQuake[nLocalPlayer] >> 7) % 31);

        if (bCamera)
        {
            viewz -= 2560;
            if (!calcChaseCamPos(&playerX, &playerY, &viewz, pPlayerSprite, &nSector, nAngle, pan, smoothRatio))
            {
                viewz += 2560;
                calcChaseCamPos(&playerX, &playerY, &viewz, pPlayerSprite, &nSector, nAngle, pan, smoothRatio);
            }
        }
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
    }

    UpdateMap();

    if (nFreeze != 3)
    {
        TArray<uint8_t> paldata(numsectors * 2 + numwalls, true);
        uint8_t* sectorFloorPal = &paldata[0];
        uint8_t* sectorCeilingPal = &paldata[numsectors];
        uint8_t* wallPal = &paldata[2*numsectors];
        int const viewingRange = viewingrange;
        int const vr = xs_CRoundToInt(65536. * tan(r_fov * (pi::pi() / 360.)));


        videoSetCorrectedAspect();
        renderSetAspect(MulScale(vr, viewingrange, 16), yxaspect);

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

        if (!testnewrenderer)
        {
            renderSetRollAngle((float)rotscrnang.asbuildf());
            renderDrawRoomsQ16(nCamerax, nCameray, viewz, nCameraa.asq16(), nCamerapan.asq16(), nSector, false);
            analyzesprites(pm_tsprite, pm_spritesortcnt, nCamerax, nCameray, viewz, smoothRatio);
            renderDrawMasks();
        }
        else
        {
            render_drawrooms(nullptr, { nCamerax, nCameray, viewz }, nSector, nCameraa, nCamerapan, rotscrnang, smoothRatio);
        }

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

        if (nFreeze)
        {
            nSnakeCam = -1;

            if (nFreeze == 2)
            {
                if (nHeadStage == 4)
                {
                    nHeadStage = 5;

                    pPlayerSprite->cstat |= 0x8000;

                    int ang2 = nCameraa.asbuild() - pPlayerSprite->ang;
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
                        subtitleOverlay.ReadyCinemaText(currentLevel->ex_ramses_text);
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
                DrawMap(smoothRatio);
            }
            else
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    pEnemy->s().pal = (uint8_t)nEnemyPal;
                }

                DrawMap(smoothRatio);
            }
        }
    }
    else
    {
        twod->ClearScreen();
    }

    pPlayerSprite->cstat = nPlayerOldCstat;
    pDop->cstat = nDoppleOldCstat;
    RestoreInterpolations();

    flash = 0;
}

bool GameInterface::GenerateSavePic()
{
    DrawView(65536, true);
    return true;
}

void GameInterface::processSprites(spritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio)
{
    analyzesprites(tsprite, spritesortcnt, viewx, viewy, viewz, smoothRatio);
}


void NoClip()
{
}

void Clip()
{
}

void SerializeView(FSerializer& arc)
{
    if (arc.BeginObject("view"))
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
            ("enemy", pEnemy)
            ("enemypal", nEnemyPal)
            .Array("vertpan", dVertPan, countof(dVertPan))
            .Array("quake", nQuake, countof(nQuake))
            .EndObject();
    }
}

END_PS_NS
