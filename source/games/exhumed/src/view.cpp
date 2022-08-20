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

BEGIN_PS_NS

bool bSubTitles = true;

int zbob;

int16_t dVertPan[kMaxPlayers];
int nCamerax;
int nCameray;
int nCameraz;


bool bTouchFloor;

int16_t nQuake[kMaxPlayers] = { 0 };

int nChunkTotal = 0;

DAngle nCameraa;
fixedhoriz nCamerapan;
int nViewTop;
bool bCamera = false;

int viewz;


// We cannot drag these through the entire event system... :(
tspriteArray* mytspriteArray;

// NOTE - not to be confused with Ken's analyzesprites()
static void analyzesprites(tspriteArray& tsprites, int x, int y, int z, double const smoothratio)
{
    mytspriteArray = &tsprites;

    for (unsigned i = 0; i < tsprites.Size(); i++) 
    {
        auto pTSprite = tsprites.get(i);

        if (pTSprite->ownerActor)
        {
            // interpolate sprite position
            pTSprite->pos = pTSprite->ownerActor->interpolatedvec3(smoothratio);
            pTSprite->angle = pTSprite->ownerActor->interpolatedang(smoothratio);
        }
    }

    auto pPlayerActor = PlayerList[nLocalPlayer].pActor;

    int var_38 = 20;
    int var_2C = 30000;


    bestTarget = nullptr;

    auto pSector =pPlayerActor->sector();

    int nAngle = (2048 - pPlayerActor->int_ang()) & kAngleMask;

    for (int nTSprite = int(tsprites.Size()-1); nTSprite >= 0; nTSprite--)
    {
        auto pTSprite = tsprites.get(nTSprite);
        auto pActor = static_cast<DExhumedActor*>(pTSprite->ownerActor);

        if (pTSprite->sectp != nullptr)
        {
            sectortype *pTSector = pTSprite->sectp;
            int nSectShade = (pTSector->ceilingstat & CSTAT_SECTOR_SKY) ? pTSector->ceilingshade : pTSector->floorshade;
            int nShade = pTSprite->shade + nSectShade + 6;
            pTSprite->shade = clamp(nShade, -128, 127);
        }

        pTSprite->pal = RemapPLU(pTSprite->pal);

        // PowerSlaveGDX: Torch bouncing fix
        if ((pTSprite->picnum == kTorch1 || pTSprite->picnum == kTorch2) && (pTSprite->cstat & CSTAT_SPRITE_YCENTER) == 0)
        {
            pTSprite->cstat |= CSTAT_SPRITE_YCENTER;
            double nTileY = (tileHeight(pTSprite->picnum) * pTSprite->yrepeat) * 2 * zinttoworld;
            pTSprite->pos.Z -= nTileY;
        }

        if (pTSprite->pal == 4 && pTSprite->shade >= numshades) pTSprite->shade = numshades - 1;

        if (pActor->spr.statnum > 0)
        {
            RunListEvent ev{};
            ev.pTSprite = pTSprite;
            runlist_SignalRun(pActor->spr.lotag - 1, nTSprite, &ExhumedAI::Draw, &ev);

            if ((pActor->spr.statnum < 150) && (pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) && (pActor != pPlayerActor))
            {
                int xval = pActor->int_pos().X - x;
                int yval = pActor->int_pos().Y - y;

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
    auto targ = bestTarget;
    if (targ != nullptr)
    {
        nCreepyTimer = kCreepyCount;

        if (!cansee(x, y, z, pSector, targ->int_pos().X, targ->int_pos().Y, targ->int_pos().Z - GetActorHeight(targ), targ->sector()))
        {
            bestTarget = nullptr;
        }
    }

    mytspriteArray = nullptr;

}

void ResetView()
{
    EraseScreen(0);
#ifdef USE_OPENGL
    videoTintBlood(0, 0, 0);
#endif
}

static TextOverlay subtitleOverlay;

void DrawView(double smoothRatio, bool sceneonly)
{
    DExhumedActor* pEnemy = nullptr;
    int nEnemyPal = -1;
    int playerX;
    int playerY;
    int playerZ;
    sectortype* pSector = nullptr;
    DAngle nAngle, rotscrnang;
    fixedhoriz pan = {};

    zbob = bsin(2 * bobangle, -3);

    DoInterpolations(smoothRatio / 65536.);

    auto pPlayerActor = PlayerList[nLocalPlayer].pActor;
    auto nPlayerOldCstat = pPlayerActor->spr.cstat;
    auto pDop = PlayerList[nLocalPlayer].pDoppleSprite;
    auto nDoppleOldCstat = pDop->spr.cstat;

    if (nSnakeCam >= 0 && !sceneonly)
    {
        DExhumedActor* pActor = SnakeList[nSnakeCam].pSprites[0];

        playerX = pActor->int_pos().X;
        playerY = pActor->int_pos().Y;
        playerZ = pActor->int_pos().Z;
        pSector = pActor->sector();
        nAngle = pActor->spr.angle;
        rotscrnang = nullAngle;

        SetGreenPal();

        pEnemy = SnakeList[nSnakeCam].pEnemy;

        if (pEnemy == nullptr || totalmoves & 1)
        {
            nEnemyPal = -1;
        }
        else
        {
            nEnemyPal = pEnemy->spr.pal;
            pEnemy->spr.pal = 5;
        }
    }
    else
    {
        playerX = pPlayerActor->__interpolatedx(smoothRatio);
        playerY = pPlayerActor->__interpolatedy(smoothRatio);
        playerZ = pPlayerActor->__interpolatedz(smoothRatio) + interpolatedvalue(PlayerList[nLocalPlayer].oeyelevel, PlayerList[nLocalPlayer].eyelevel, smoothRatio);

        pSector = PlayerList[nLocalPlayer].pPlayerViewSect;
        updatesector(playerX, playerY, &pSector);
        if (pSector == nullptr) pSector = PlayerList[nLocalPlayer].pPlayerViewSect;

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
            pPlayerActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            pDop->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        }
        else
        {
            pPlayerActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
            pDop->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
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
        int floorZ = pPlayerActor->sector()->int_floorz();

        if (viewz > floorZ)
            viewz = floorZ;

        nCameraa += DAngle::fromBuild((nQuake[nLocalPlayer] >> 7) % 31);

        if (bCamera)
        {
            viewz -= 2560;
            if (!calcChaseCamPos(&playerX, &playerY, &viewz, pPlayerActor, &pSector, nAngle, pan, smoothRatio))
            {
                viewz += 2560;
                calcChaseCamPos(&playerX, &playerY, &viewz, pPlayerActor, &pSector, nAngle, pan, smoothRatio);
            }
        }
    }
    nCamerax = playerX;
    nCameray = playerY;
    nCameraz = playerZ;

    if (pSector != nullptr)
    {
        int Z = pSector->int_ceilingz() + 256;
        if (Z <= viewz)
        {
            Z = pSector->int_floorz() - 256;

            if (Z < viewz)
                viewz = Z;
        }
        else {
            viewz = Z;
        }
    }

    nCamerapan = pan;

    if (nFreeze == 2 || nFreeze == 1)
    {
        nSnakeCam = -1;
        //???
        viewport3d = { 0, 0, screen->GetWidth(), screen->GetHeight() };
    }

    UpdateMap();

    if (nFreeze != 3)
    {
        TArray<uint8_t> paldata(sector.Size() * 2 + wall.Size(), true);

        if (HavePLURemap())
        {
            auto p = paldata.Data();
            for (auto& sect: sector)
            {
                uint8_t v;
                v = *p++ = sect.floorpal;
                sect.floorpal = RemapPLU(v);
                v = *p++ = sect.ceilingpal;
                sect.ceilingpal = RemapPLU(v);
            }
            for (auto& wal : wall)
            {
                uint8_t v;
                v = *p++ = wal.pal;
                wal.pal = RemapPLU(v);
            }
        }

        if (!nFreeze && !sceneonly)
            DrawWeapons(smoothRatio);
        render_drawrooms(nullptr, { nCamerax, nCameray, viewz }, sectnum(pSector), nCameraa, nCamerapan, rotscrnang, smoothRatio);

        if (HavePLURemap())
        {
            auto p = paldata.Data();
            for (auto& sect: sector)
            {
                sect.floorpal = *p++;
                sect.ceilingpal = *p++;
            }
            for (auto& wal : wall)
            {
                wal.pal = *p++;
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

                    pPlayerActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;

                    int ang2 = nCameraa.Buildang() - pPlayerActor->int_ang();
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
                DrawMap(smoothRatio);
            }
            else
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    pEnemy->spr.pal = (uint8_t)nEnemyPal;
                }

                DrawMap(smoothRatio);
            }
        }
    }
    else
    {
        twod->ClearScreen();
    }

    pPlayerActor->spr.cstat = nPlayerOldCstat;
    pDop->spr.cstat = nDoppleOldCstat;
    RestoreInterpolations();

    flash = 0;
}

bool GameInterface::GenerateSavePic()
{
    DrawView(65536, true);
    return true;
}

void GameInterface::processSprites(tspriteArray& tsprites, int viewx, int viewy, int viewz, DAngle viewang, double smoothRatio)
{
    analyzesprites(tsprites, viewx, viewy, viewz, smoothRatio);
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
            .Array("vertpan", dVertPan, countof(dVertPan))
            .Array("quake", nQuake, countof(nQuake))
            .EndObject();
    }
}

END_PS_NS
