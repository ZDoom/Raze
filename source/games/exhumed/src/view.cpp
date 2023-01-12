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

int16_t dVertPan[kMaxPlayers];
DVector3 nCamerapos;
bool bTouchFloor;

double nQuake[kMaxPlayers] = { 0 };

int nChunkTotal = 0;

int nViewTop;
bool bCamera = false;



// We cannot drag these through the entire event system... :(
tspriteArray* mytspriteArray;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void analyzesprites(tspriteArray& tsprites, const DVector3& view, double const interpfrac)
{
    mytspriteArray = &tsprites;

    for (unsigned i = 0; i < tsprites.Size(); i++) 
    {
        auto pTSprite = tsprites.get(i);

        if (pTSprite->ownerActor)
        {
            // interpolate sprite position
            pTSprite->pos = pTSprite->ownerActor->interpolatedpos(interpfrac);
            pTSprite->Angles.Yaw = pTSprite->ownerActor->interpolatedyaw(interpfrac);
        }
    }

    auto pPlayerActor = PlayerList[nLocalPlayer].pActor;

    double bestclose = 20;
    double bestside = 30000;


    bestTarget = nullptr;

    auto pSector =pPlayerActor->sector();

    DAngle nAngle = -pPlayerActor->spr.Angles.Yaw;

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
            auto tex = TexMan.GetGameTexture(pTSprite->spritetexture());
            double nTileY = (tex->GetDisplayHeight() * pTSprite->scale.Y) * 0.5;
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
                DVector2 delta = pActor->spr.pos.XY() - view.XY();

                double vcos = nAngle.Cos();
                double vsin = nAngle.Sin();


                double fwd = ((vcos * delta.Y) + (delta.X * vsin));
                double side = abs((vcos * delta.X) - (delta.Y * vsin));

                if (!side)
                    continue;

                double close = (abs(fwd) * 32) / side;
                if (side < 1000 / 16. && side < bestside && close < 10)
                {
                    bestTarget = pActor;
                    bestclose = close;
                    bestside = side;
                }
                else if (side < 30000 / 16.)
                {
                    double t = bestclose - close;
                    if (t > 3 || (side < bestside && abs(t) < 5))
                    {
                        bestclose = close;
                        bestside = side;
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

        if (!cansee(view, pSector, targ->spr.pos.plusZ(-GetActorHeight(targ)), targ->sector()))
        {
            bestTarget = nullptr;
        }
    }

    mytspriteArray = nullptr;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ResetView()
{
    EraseScreen(0);
    videoTintBlood(0, 0, 0);
}

static TextOverlay subtitleOverlay;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawView(double interpfrac, bool sceneonly)
{
    DExhumedActor* pEnemy = nullptr;
    int nEnemyPal = -1;
    sectortype* pSector = nullptr;
    DRotator nCameraangles{};

    DoInterpolations(interpfrac);

    auto pPlayer = &PlayerList[nLocalPlayer];
    auto pPlayerActor = pPlayer->pActor;
    auto nPlayerOldCstat = pPlayerActor->spr.cstat;
    auto pDop = pPlayer->pDoppleSprite;
    auto nDoppleOldCstat = pDop->spr.cstat;

    // update render angles.
    pPlayer->Angles.updateRenderAngles(interpfrac);
    UpdatePlayerSpriteAngle(pPlayer);

    if (nSnakeCam >= 0 && !sceneonly)
    {
        DExhumedActor* pActor = SnakeList[nSnakeCam].pSprites[0];

        nCamerapos = pActor->spr.pos;
        pSector = pActor->sector();
        nCameraangles.Yaw = pActor->spr.Angles.Yaw;

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
        nCamerapos = pPlayerActor->getRenderPos(interpfrac);

        pSector = PlayerList[nLocalPlayer].pPlayerViewSect;
        updatesector(nCamerapos, &pSector);
        if (pSector == nullptr) pSector = PlayerList[nLocalPlayer].pPlayerViewSect;

        nCameraangles = PlayerList[nLocalPlayer].Angles.getRenderAngles(interpfrac);

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
    }

    if (nSnakeCam >= 0 && !sceneonly)
    {
        nCameraangles.Pitch = nullAngle;
    }
    else
    {
        nCamerapos.Z = min(nCamerapos.Z + nQuake[nLocalPlayer], pPlayerActor->sector()->floorz);
        nCameraangles.Yaw += DAngle::fromDeg(fmod(nQuake[nLocalPlayer], 16.) * (45. / 128.));

        if (bCamera)
        {
            nCamerapos.Z -= 10;
            if (!calcChaseCamPos(nCamerapos, pPlayerActor, &pSector, nCameraangles, interpfrac, 96.))
            {
                nCamerapos.Z += 10;
                calcChaseCamPos(nCamerapos, pPlayerActor, &pSector, nCameraangles, interpfrac, 96.);
            }
        }
    }

    if (pSector != nullptr)
    {
        nCamerapos.Z = min(max(nCamerapos.Z, pSector->ceilingz + 1), pSector->floorz - 1);
    }

    if (nFreeze == 2 || nFreeze == 1)
    {
        nSnakeCam = -1;
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
            DrawWeapons(interpfrac);
        render_drawrooms(nullptr, nCamerapos, pSector, nCameraangles, interpfrac);

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

                    auto ang2 = nCameraangles.Yaw - pPlayerActor->spr.Angles.Yaw;
                    if (ang2.Degrees() < 0)
                        ang2 = -ang2;

                    if (ang2 > mapangle(10))
                    {
                        inita -= ang2 * (1. / 8.);
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
            if (nSnakeCam >= 0)
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    pEnemy->spr.pal = (uint8_t)nEnemyPal;
                }
            }

            DrawMap(nCamerapos.XY(), nCameraangles.Yaw, interpfrac);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::GenerateSavePic()
{
    DrawView(65536, true);
    return true;
}

void GameInterface::processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac)
{
    analyzesprites(tsprites, view, interpfrac);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeView(FSerializer& arc)
{
    if (arc.BeginObject("view"))
    {
        arc("camerapos", nCamerapos)
            ("touchfloor", bTouchFloor)
            ("chunktotal", nChunkTotal)
            ("camera", bCamera)
            .Array("vertpan", dVertPan, countof(dVertPan))
            .Array("quake", nQuake, countof(nQuake))
            .EndObject();
    }
}

END_PS_NS
