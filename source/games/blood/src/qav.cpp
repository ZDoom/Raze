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
#include "v_2ddrawer.h"
#include "compat.h"
#include "common_game.h"
#include "v_draw.h"
#include "blood.h"

BEGIN_BLD_NS

extern void (*qavClientCallback[])(int, void *);


//==========================================================================
//
// QAV interpolation functions
//
//==========================================================================

enum
{
    kQAVIsLoopable,
};

static TMap<FString, QAVPrevTileFinder> qavPrevTileFinders;
static TMap<int, TMap<int, TArray<int>>> qavSkippedFrameTiles;
static TMap<int, QAVInterpProps> qavInterpProps;

static void qavInitTileFinderMap()
{
    // Interpolate between frames if the picnums match. This is safest but could miss interpolations between suitable picnums.
    qavPrevTileFinders.Insert("interpolate-picnum", [](FRAMEINFO* const thisFrame, FRAMEINFO* const prevFrame, const int& i) -> TILE_FRAME* {
        return prevFrame->tiles[i].picnum == thisFrame->tiles[i].picnum ? &prevFrame->tiles[i] : nullptr;
    });

    // Interpolate between frames if the picnum is valid. This can be problematic if tile indices change between frames.
    qavPrevTileFinders.Insert("interpolate-index", [](FRAMEINFO* const thisFrame, FRAMEINFO* const prevFrame, const int& i) -> TILE_FRAME* {
        return prevFrame->tiles[i].picnum > 0 ? &prevFrame->tiles[i] : nullptr;
    });

    // Find previous frame by iterating all previous frame's tiles and return on first matched x coordinate.
    qavPrevTileFinders.Insert("interpolate-x", [](FRAMEINFO* const thisFrame, FRAMEINFO* const prevFrame, const int& i) -> TILE_FRAME* {
        for (int j = 0; j < 8; j++) if (thisFrame->tiles[i].x == prevFrame->tiles[j].x)
        {
            return &prevFrame->tiles[j];
        }
        return nullptr;
    });

    // Find previous frame by iterating all previous frame's tiles and return on first matched y coordinate.
    qavPrevTileFinders.Insert("interpolate-y", [](FRAMEINFO* const thisFrame, FRAMEINFO* const prevFrame, const int& i) -> TILE_FRAME* {
        for (int j = 0; j < 8; j++) if (thisFrame->tiles[i].y == prevFrame->tiles[j].y)
        {
            return &prevFrame->tiles[j];
        }
        return nullptr;
    });

    // When type is unspecified, default to using the safest interpolation option.
    qavPrevTileFinders.Insert("interpolate", *qavPrevTileFinders.CheckKey("interpolate-picnum"));
}

static bool qavCanInterpFrameTile(const int& res_id, const int& nFrame, const int& i)
{
    // Check whether this QAV has skippable tiles.
    auto thisQAV = qavSkippedFrameTiles.CheckKey(res_id);
    if (thisQAV)
    {
        // Check whether the current frame's tile is skippable.
        auto thisFrame = thisQAV->CheckKey(nFrame);
        if (thisFrame)
        {
            return !thisFrame->Contains(i);
        }
    }

    // Return true by default.
    return true;
}

QAVPrevTileFinder qavGetInterpType(const FString& type)
{
    if (!qavPrevTileFinders.CountUsed()) qavInitTileFinderMap();
    return *qavPrevTileFinders.CheckKey(type);
}

void qavSetNonInterpFrameTile(const int& res_id, const int& nFrame, const int& i)
{
    // Check whether incoming resource is already in TMap.
    auto thisQAV = qavSkippedFrameTiles.CheckKey(res_id);
    if (!thisQAV)
    {
        TMap<int, TArray<int>> framemap;
        qavSkippedFrameTiles.Insert(res_id, std::move(framemap));
        thisQAV = qavSkippedFrameTiles.CheckKey(res_id);
    }

    // Check whether this resource's TMap has a frame TMap.
    auto thisFrame = thisQAV->CheckKey(nFrame);
    if (!thisFrame)
    {
        TArray<int> tilearray;
        thisQAV->Insert(nFrame, std::move(tilearray));
        thisFrame = thisQAV->CheckKey(nFrame);
    }

    // Check whether the TArray in this frame's TMap already contains the tile.
    if (!thisFrame->Contains(i))
    {
        thisFrame->Push(i);
    }

    return;
}

void qavBuildInterpProps(QAV* const pQAV)
{
    switch (pQAV->res_id)
    {
        case kQAVBDRIP:
        {
            QAVInterpProps interp{};
            interp.flags |= true << kQAVIsLoopable;
            interp.PrevTileFinder = qavGetInterpType("interpolate-x");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            for (int i = 0; i < pQAV->nFrames; i++)
            {
                qavSetNonInterpFrameTile(pQAV->res_id, i, 0);
            }
            break;
        }
        case kQAVPFORK:
        case kQAVREMIDLE1:
        case kQAVREMIDLE2:
        case kQAVFLARFIR2:
        case kQAVTOMSPRED:
        case kQAV2TOMALT:
        case kQAVSGUNFIR1:
        case kQAV2SGUNFIR:
        case kQAVNAPFIRE:
        case kQAVVDIDLE2:
        {
            QAVInterpProps interp{};
            interp.flags |= true << kQAVIsLoopable;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            break;
        }
        case kQAVLITEFLAM:
        {
            break;
        }
        case kQAVCANFIRE2:
        {
            QAVInterpProps interp{};
            interp.flags = 0;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            for (int i = 14; i < pQAV->nFrames; i++)
            {
                for (int j = 2; j < 4; j++)
                {
                    qavSetNonInterpFrameTile(pQAV->res_id, i, j);
                }
            }
            break;
        }
        case kQAVBUNFUSE:
        {
            QAVInterpProps interp{};
            interp.flags = 0;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            for (int i = 6; i < pQAV->nFrames; i++)
            {
                qavSetNonInterpFrameTile(pQAV->res_id, i, 4);
            }
            break;
        }
        case kQAVSHOTL1:
        {
            QAVInterpProps interp{};
            interp.flags = 0;
            interp.PrevTileFinder = qavGetInterpType("interpolate-picnum");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            break;
        }
        case kQAVTOMFIRE:
        {
            QAVInterpProps interp{};
            interp.flags |= true << kQAVIsLoopable;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            for (int i = 0; i < pQAV->nFrames; i++)
            {
                qavSetNonInterpFrameTile(pQAV->res_id, i, 0);
            }
            break;
        }
        case kQAV2TOMFIRE:
        {
            QAVInterpProps interp{};
            interp.flags |= true << kQAVIsLoopable;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            for (int i = 0; i < pQAV->nFrames; i++)
            {
                for (int j = 0; j < 3; j += 2)
                {
                    qavSetNonInterpFrameTile(pQAV->res_id, i, j);
                }
            }
            break;
        }
        default:
        {
            QAVInterpProps interp{};
            interp.flags = 0;
            interp.PrevTileFinder = qavGetInterpType("interpolate-index");
            qavInterpProps.Insert(pQAV->res_id, std::move(interp));
            break;
        }
    }
}


void DrawFrame(double x, double y, double z, double a, TILE_FRAME *pTile, int stat, int shade, int palnum, bool to3dview)
{
    stat |= pTile->stat;
    if (palnum <= 0) palnum = pTile->palnum;

    if (!to3dview)
    {
		auto tex = tileGetTexture(pTile->picnum);
		double scale = z * (1. / 65536.);
		double angle = a * BAngToDegree;
		int renderstyle = (stat & RS_NOMASK)? STYLE_Normal : STYLE_Translucent;
		double alpha = (stat & RS_TRANS1)? glblend[0].def[!!(stat & RS_TRANS2)].alpha : 1.;
		int pin = (stat & kQavOrientationLeft)? -1 : (stat & RS_ALIGN_R)? 1:0;
		auto translation = TRANSLATION(Translation_Remap, palnum);
		bool topleft = !!(stat & RS_TOPLEFT);

		bool xflip = !!(stat & 0x100); // repurposed flag
		bool yflip = !!(stat & RS_YFLIP);
		auto color = shadeToLight(pTile->shade + shade);

		DrawTexture(twod, tex, x, y, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Rotate, angle, DTA_LegacyRenderStyle, renderstyle, DTA_Alpha, alpha, DTA_Pin, pin, DTA_TranslationIndex, translation,
					DTA_TopLeft, topleft, DTA_CenterOffsetRel, !topleft, DTA_FullscreenScale, FSMode_Fit320x200, DTA_FlipOffsets, true, DTA_Color, color,
					DTA_FlipX, xflip, DTA_FlipY, yflip, TAG_DONE);
    }
    else
    {
        // there's some disagreements about flag values between QAV and the drawer. Shuffle these around.
		if (stat & RS_YFLIP) stat |= RS_YFLIPHUD;
		stat &= ~RS_YFLIP;
		if (stat & 0x100) stat |= RS_XFLIPHUD;
        stat &= ~0x100;
		if ((stat & kQavOrientationLeft)) stat |= RS_ALIGN_L;
        stat &= ~kQavOrientationLeft;

		hud_drawsprite(x, y, z, a, pTile->picnum, pTile->shade + shade, palnum, stat);
    }
}

void QAV::Draw(double x, double y, int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio, bool const looped)
{
    assert(ticksPerFrame > 0);

    auto const interpdata = qavInterpProps.CheckKey(res_id);

    auto const nFrame = clamp(ticks / ticksPerFrame, 0, nFrames - 1);
    FRAMEINFO* const thisFrame = &frames[nFrame];

    auto const oFrame = clamp((nFrame == 0 && (looped || interpdata && (interpdata->flags & kQAVIsLoopable)) ? nFrames : nFrame) - 1, 0, nFrames - 1);
    FRAMEINFO* const prevFrame = &frames[oFrame];

    bool const interpolate = interpdata && cl_hudinterpolation && cl_bloodqavinterp && (nFrames > 1) && (nFrame != oFrame) && (smoothratio != MaxSmoothRatio);

    for (int i = 0; i < 8; i++)
    {
        if (thisFrame->tiles[i].picnum > 0)
        {
            TILE_FRAME* const thisTile = &thisFrame->tiles[i];
            TILE_FRAME* const prevTile = interpolate && qavCanInterpFrameTile(res_id, nFrame, i) ? interpdata->PrevTileFinder(thisFrame, prevFrame, i) : nullptr;

            double tileX = x;
            double tileY = y;
            double tileZ;
            double tileA;

            if (prevTile)
            {
                tileX += interpolatedvaluef(prevTile->x, thisTile->x, smoothratio);
                tileY += interpolatedvaluef(prevTile->y, thisTile->y, smoothratio);
                tileZ = interpolatedvaluef(prevTile->z, thisTile->z, smoothratio);
                tileA = interpolatedangle(buildang(prevTile->angle), buildang(thisTile->angle), smoothratio).asbuildf();
            }
            else
            {
                tileX += thisTile->x;
                tileY += thisTile->y;
                tileZ = thisTile->z;
                tileA = thisTile->angle;
            }

            DrawFrame(tileX, tileY, tileZ, tileA, thisTile, stat, shade, palnum, to3dview);
        }
    }
}


void QAV::Play(int start, int end, int nCallback, void *pData)
{
    assert(ticksPerFrame > 0);
    int frame;
    int ticks;
    if (start < 0)
        frame = (start + 1) / ticksPerFrame;
    else
        frame = start / ticksPerFrame + 1;
    
    for (ticks = ticksPerFrame * frame; ticks <= end; frame++, ticks += ticksPerFrame)
    {
        if (frame >= 0 && frame < nFrames)
        {
            FRAMEINFO *pFrame = &frames[frame];
            SOUNDINFO *pSound = &pFrame->sound;
            
            // by NoOne: handle Sound kill flags
            if (!VanillaMode() && pSound->sndFlags > 0 && pSound->sndFlags <= kFlagSoundKillAll) {
                for (int i = 0; i < nFrames; i++) {
                    FRAMEINFO* pFrame2 = &frames[i];
                    SOUNDINFO* pSound2 = &pFrame2->sound;
                    if (pSound2->sound != 0) {
                        if (pSound->sndFlags != kFlagSoundKillAll && pSound2->priority != pSound->priority) continue;
                        else if (nSprite >= 0) {
                            // We need stop all sounds in a range
                            for (int a = 0; a <= pSound2->sndRange; a++)
                                sfxKill3DSound(&sprite[nSprite], -1, pSound2->sound + a);
                        } else {
                            sndKillAllSounds();
                        }
                    }
                }
            }

            if (pSound->sound > 0) {
                int sound = pSound->sound;
                
                // by NoOne: add random rage sound feature
                if (pSound->sndRange > 0 && !VanillaMode()) 
                    sound += Random((pSound->sndRange == 1) ? 2 : pSound->sndRange);
                
                if (nSprite == -1) sndStartSample(sound, -1, -1, 0);
                else sfxPlay3DSound(&sprite[nSprite], sound, 16+pSound->priority, 6);
            }
            
            if (pFrame->nCallbackId > 0 && nCallback != -1) {
                qavClientCallback[nCallback](pFrame->nCallbackId, pData);
            }
        }
    }
}

void QAV::Precache(int palette)
{
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (frames[i].tiles[j].picnum >= 0)
                tilePrecacheTile(frames[i].tiles[j].picnum, 0, palette);
        }
    }
}

void qavProcessTicker(QAV* const pQAV, int* duration, int* lastTick)
{
    if (*duration > 0)
    {
        auto thisTick = I_GetTime(pQAV->ticrate);
        auto numTicks = thisTick - (*lastTick);
        if (numTicks)
        {
            *lastTick = thisTick;
            *duration -= pQAV->ticksPerFrame * numTicks;
        }
    }
    *duration = ClipLow(*duration, 0);
}

void qavProcessTimer(PLAYER* const pPlayer, QAV* const pQAV, int* duration, double* smoothratio, bool const fixedduration)
{
    // Process clock based on QAV's ticrate and last tick value.
    qavProcessTicker(pQAV, &pPlayer->qavTimer, &pPlayer->qavLastTick);

    if (pPlayer->weaponTimer == 0)
    {
        // Check if we're playing an idle QAV as per the ticker's weapon timer.
        *duration = fixedduration ? pQAV->duration - 1 : I_GetBuildTime() % pQAV->duration;
        *smoothratio = MaxSmoothRatio;
    }
    else if (pPlayer->qavTimer == 0)
    {
        // If qavTimer is 0, play the last frame uninterpolated. Sometimes the timer can be just ahead of weaponTimer.
        *duration = pQAV->duration - 1;
        *smoothratio = MaxSmoothRatio;
    }
    else
    {
        // Apply normal values.
        *duration = pQAV->duration - pPlayer->qavTimer;
        *smoothratio = I_GetTimeFrac(pQAV->ticrate) * MaxSmoothRatio;
    }
}

static void qavRepairTileData(QAV* pQAV)
{
    int i, j, lastframe;
    TILE_FRAME backup;

    switch (pQAV->res_id)
    {
        case kQAVCANDOWN:
            // CANDOWN interpolates fine, but the starting frame in bringing the can down is lower than the can while idle.
            // Do linear interpolation from 2nd last frame through to first frame, ending with coordinates of CANIDLE.
            lastframe = pQAV->nFrames - 1;
            for (i = lastframe, j = 0; i >= 0; i--, j++)
            {
                pQAV->frames[j].tiles[2].x = xs_CRoundToInt(pQAV->frames[lastframe].tiles[2].x - (double(pQAV->frames[lastframe].tiles[2].x - 11) / lastframe) * i);
                pQAV->frames[j].tiles[2].y = xs_CRoundToInt(pQAV->frames[lastframe].tiles[2].y - (double(pQAV->frames[lastframe].tiles[2].y - -28) / lastframe) * i);
            }
            break;
        case kQAVCANFIRE2:
            // Handle some index swaps and cripple interpolation after 14th frame.
            // Swap tile indices 1 and 2 around for frame's 0 and 1.
            for (i = 0; i < 2; i++)
            {
                backup = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = backup;
            }

            // Move what's now frame 0 tile 2 to tile 3 and disable original index of 2;
            pQAV->frames[0].tiles[3] = pQAV->frames[0].tiles[2];
            pQAV->frames[0].tiles[2].picnum = -1;

            // For frame 11 until the end, move frame indices 0 and 1 to 2 and 3 respectively, and disable the original indices.
            for (i = 11; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0].picnum = -1;
                pQAV->frames[i].tiles[3] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }
            break;
        case kQAVBUNUP:
            // BUNUP has several tile indices that require repairs here to minimise continual checks at draw time.
            // For the 4th frame, clone tile indices 5 and 6 into 2 and 3 respectively where they should have been.
            for (i = 5; i < 7; i++)
            {
                pQAV->frames[3].tiles[i - 3] = pQAV->frames[3].tiles[i];
                pQAV->frames[3].tiles[i].picnum = -1;
            }

            // For the 2nd frame, clone tile indices 3 and 4 into 2 and 3 respectively where they should have been.
            for (i = 3; i < 5; i++)
            {
                pQAV->frames[1].tiles[i - 1] = pQAV->frames[1].tiles[i];
            }

            // Clone 1st frame's tile index 2 to tile index 4 for 1st and 2nd frame, then disable original index of 2.
            pQAV->frames[1].tiles[4] = pQAV->frames[0].tiles[4] = pQAV->frames[0].tiles[2];
            pQAV->frames[0].tiles[2].picnum = -1;

            // Clone 1st frame's tile index 0 to tile index 3, then disable original index of 0.
            pQAV->frames[0].tiles[3] = pQAV->frames[0].tiles[0];
            pQAV->frames[0].tiles[0].picnum = -1;

            // Shift every tile up one index to leave more room at the end, should it be needed in the future.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                for (j = 1; j < 5; j++)
                {
                    pQAV->frames[i].tiles[j - 1] = pQAV->frames[i].tiles[j];
                    pQAV->frames[i].tiles[j].picnum = -1;
                }
            }
            break;
        case kQAVBUNDOWN:
            // BUNDOWN requires some tile index swaps to be cleaned up to avoid using our own callback.
            // For frames 3 till the end, backup tile index 3, move indices 1 and 2 down, then restore backed up tile index 3 as 1.
            for (i = 3; i < pQAV->nFrames; i++)
            {
                backup = pQAV->frames[i].tiles[3];
                pQAV->frames[i].tiles[3] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = backup;
            }
            break;
        case kQAVBUNUP2:
            // BUNUP2 has a few uninterpolatable tiles that need moving, and some index swaps to handle.
            // For frame 2, move tile index 1 to 3 and disable original index of 1.
            pQAV->frames[2].tiles[3] = pQAV->frames[2].tiles[1];
            pQAV->frames[2].tiles[1].picnum = -1;

            // For frame 7, move tile index 1 into 6, 2 into 1 and 3 into 2, then disable the original index of 3.
            pQAV->frames[7].tiles[6] = pQAV->frames[7].tiles[1];
            pQAV->frames[7].tiles[1] = pQAV->frames[7].tiles[2];
            pQAV->frames[7].tiles[2] = pQAV->frames[7].tiles[3];
            pQAV->frames[7].tiles[3].picnum = -1;

            // For frame 8, move tile index 1 into 5, 2 into 6, 3 into 1 and 4 into 2, then disable the original index of 4.
            pQAV->frames[8].tiles[5] = pQAV->frames[8].tiles[1];
            pQAV->frames[8].tiles[6] = pQAV->frames[8].tiles[2];
            pQAV->frames[8].tiles[1] = pQAV->frames[8].tiles[3];
            pQAV->frames[8].tiles[2] = pQAV->frames[8].tiles[4];
            pQAV->frames[8].tiles[4].picnum = -1;

            // For frame 9, move tile index 1 into 5, 2 into 1 and 3 into 2, then disable the original index of 3.
            pQAV->frames[9].tiles[5] = pQAV->frames[9].tiles[1];
            pQAV->frames[9].tiles[1] = pQAV->frames[9].tiles[2];
            pQAV->frames[9].tiles[2] = pQAV->frames[9].tiles[3];
            pQAV->frames[9].tiles[3].picnum = -1;

            // For frames 7 until the end, move indices 5 and 6 into 3 and 4, and disable original indices of 5 and 6.
            for (i = 7; i < pQAV->nFrames; i++)
            {
                for (j = 5; j < 7; j++)
                {
                    pQAV->frames[i].tiles[j - 2] = pQAV->frames[i].tiles[j];
                    pQAV->frames[i].tiles[j].picnum = -1; 
                }
            }
            break;
        case kQAVBUNDOWN2:
            // BUNDOWN2 has some tile index swaps that require handling.
            // For frames 3 and 4, move tile indices 1 and 2 into 2 and 3, and disable original index of 1.
            for (i = 3; i < 5; i++)
            {
                pQAV->frames[i].tiles[3] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }

            // For frame 5, move tile index 1 to 3 and disable original index of 1.
            pQAV->frames[5].tiles[3] = pQAV->frames[5].tiles[1];
            pQAV->frames[5].tiles[1].picnum = -1;
            break;
        case kQAVBUNFUSE:
            // BUNFUSE has several tile indices that require repairs here to minimise continual checks at draw time.
            // For frame 0, move tile indices 2 and 3 into 3 and 4, and disable original index of 2.
            pQAV->frames[0].tiles[4] = pQAV->frames[0].tiles[3];
            pQAV->frames[0].tiles[3] = pQAV->frames[0].tiles[2];
            pQAV->frames[0].tiles[2].picnum = -1;

            // For frame 1, move tile indices 4 and 5 into 3 and 4, and disable original index of 5.
            pQAV->frames[1].tiles[3] = pQAV->frames[1].tiles[4];
            pQAV->frames[1].tiles[4] = pQAV->frames[1].tiles[5];
            pQAV->frames[1].tiles[5].picnum = -1;

            // For frame 2, move tile indices 5 and 7 into 3 and 4, and disable original indices.
            pQAV->frames[2].tiles[3] = pQAV->frames[2].tiles[5];
            pQAV->frames[2].tiles[4] = pQAV->frames[2].tiles[7];
            pQAV->frames[2].tiles[5].picnum = -1;
            pQAV->frames[2].tiles[7].picnum = -1;

            // For frames 0-5, swap tile indices 2 and 4 around.
            for (i = 0; i < 6; i++)
            {
                backup = pQAV->frames[i].tiles[4];
                pQAV->frames[i].tiles[4] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2] = backup;
            }
            break;
        case kQAVBUNDROP:
            // BUNDROP needs frame 3 tile 1 moved to tile 2 to avoid needing its own interpolation callback.
            // For frame 3, move tile index 2 into 3, and disable original index of 2.
            pQAV->frames[3].tiles[2] = pQAV->frames[3].tiles[1];
            pQAV->frames[3].tiles[1].picnum = -1;
            break;
        case kQAVBUNTHRO:
            // BUNTHRO has several tile indices that require repairs here to minimise continual checks at draw time.
            // For frame 3, move tile indices 0 and 1 into 3 and 2, and disable original indices.
            pQAV->frames[3].tiles[3] = pQAV->frames[3].tiles[0];
            pQAV->frames[3].tiles[2] = pQAV->frames[3].tiles[1];
            pQAV->frames[3].tiles[0].picnum = -1;
            pQAV->frames[3].tiles[1].picnum = -1;
            break;
        case kQAVPROXUP:
            // PROXUP has several tile indices that require repairs to avoid needing its own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 3, move tile indices 1, 2 and 3 into 0, 1 and 2, and disable original index of 3.
            for (i = 0; i < 3; i++)
            {
                pQAV->frames[3].tiles[i] = pQAV->frames[3].tiles[i + 1];
            }
            pQAV->frames[3].tiles[3].picnum = -1;

            // For frame 0, move tile index 0 into 1.
            pQAV->frames[0].tiles[1] = pQAV->frames[0].tiles[0];

            // For frame 0, clone frame 1's tile indices 0 and 2 and adjust x/y coordinates.
            // using difference between frame 0 and 1's tile index 1.
            for (i = 0; i < 3; i += 2)
            {
                pQAV->frames[0].tiles[i] = pQAV->frames[1].tiles[i];
                pQAV->frames[0].tiles[i].x += pQAV->frames[0].tiles[1].x - pQAV->frames[1].tiles[1].x;
                pQAV->frames[0].tiles[i].y += pQAV->frames[0].tiles[1].y - pQAV->frames[1].tiles[1].y;
            }
            break;
        case kQAVPROXDOWN:
            // PROXUP has tile index that require repairs to avoid needing its own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 4, move tile index 0 into 1.
            pQAV->frames[4].tiles[1] = pQAV->frames[4].tiles[0];

            // For frame 4, clone frame 3's tile indices 0 and 2 and adjust x/y coordinates.
            // using difference between frame 4 and 3's tile index 1.
            for (i = 0; i < 3; i += 2)
            {
                pQAV->frames[4].tiles[i] = pQAV->frames[3].tiles[i];
                pQAV->frames[4].tiles[i].x += pQAV->frames[4].tiles[1].x - pQAV->frames[3].tiles[1].x;
                pQAV->frames[4].tiles[i].y += pQAV->frames[4].tiles[1].y - pQAV->frames[3].tiles[1].y;
            }
            break;
        case kQAVREMUP1:
        case kQAVREMUP2:
            // REMUP1 and REMUP2 have several tile indices that require repairs to avoid needing their own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 0, move tile index 1 into 2, and disable original index of 1.
            pQAV->frames[0].tiles[2] = pQAV->frames[0].tiles[1];
            pQAV->frames[0].tiles[1].picnum = -1;

            // For frame 0, clone frame 1 tile index 1 and adjust x/y coordinates
            // using difference between frame 0 and 1's tile index 0.
            pQAV->frames[0].tiles[1] = pQAV->frames[1].tiles[1];
            pQAV->frames[0].tiles[1].x += pQAV->frames[0].tiles[0].x - pQAV->frames[1].tiles[0].x;
            pQAV->frames[0].tiles[1].y += pQAV->frames[0].tiles[0].y - pQAV->frames[1].tiles[0].y;

            // For frame 2, move tile index 2 and three around.
            backup = pQAV->frames[2].tiles[2];
            pQAV->frames[2].tiles[2] = pQAV->frames[2].tiles[3];
            pQAV->frames[2].tiles[3] = backup;

            // For frame 1, clone frame 2 tile index 3 and adjust x/y coordinates
            // using difference between frame 1 and 2's tile index 0.
            pQAV->frames[1].tiles[3] = pQAV->frames[2].tiles[3];
            pQAV->frames[1].tiles[3].x += pQAV->frames[1].tiles[1].x - pQAV->frames[2].tiles[1].x;
            pQAV->frames[1].tiles[3].y += pQAV->frames[1].tiles[1].y - pQAV->frames[2].tiles[1].y;
            break;
        case kQAVREMDOWN1:
            // REMDOWN1 has several tile indices that require repairs to avoid needing its own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 1, move tile index 2 and 3 around.
            backup = pQAV->frames[1].tiles[2];
            pQAV->frames[1].tiles[2] = pQAV->frames[1].tiles[3];
            pQAV->frames[1].tiles[3] = backup;

            // For frame 0, clone frame 1 tile index 3 and adjust x/y coordinates
            // using difference between frame 0 and 1's tile index 1.
            pQAV->frames[0].tiles[3] = pQAV->frames[1].tiles[3];
            pQAV->frames[0].tiles[3].x += pQAV->frames[0].tiles[1].x - pQAV->frames[1].tiles[1].x;
            pQAV->frames[0].tiles[3].y += pQAV->frames[0].tiles[1].y - pQAV->frames[1].tiles[1].y;

            // For frame 4, move tile index 1 into 2.
            pQAV->frames[4].tiles[2] = pQAV->frames[4].tiles[1];

            // For frame 5, move tile index 0 into 2.
            pQAV->frames[5].tiles[2] = pQAV->frames[5].tiles[0];

            // Clone frame 3 tile index 0 and 1 into frames 4 and 5, and adjust x/y coordinates
            // using difference between frames 4 and 3 and 5 and 4 on looped tile index.
            for (i = 4; i < 6; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    pQAV->frames[i].tiles[j] = pQAV->frames[i - 1].tiles[j];
                    pQAV->frames[i].tiles[j].x += pQAV->frames[i - 1].tiles[j].x - pQAV->frames[i - 2].tiles[j].x;
                    pQAV->frames[i].tiles[j].y += pQAV->frames[i - 1].tiles[j].y - pQAV->frames[i - 2].tiles[j].y;
                }
            }
            break;
        case kQAVREMDOWN2:
            // REMDOWN2 has several tile indices that require repairs to avoid needing its own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 1, move tile index 2 and 3 around.
            backup = pQAV->frames[1].tiles[2];
            pQAV->frames[1].tiles[2] = pQAV->frames[1].tiles[3];
            pQAV->frames[1].tiles[3] = backup;

            // For frame 0, clone frame 1 tile index 3 and adjust x/y coordinates
            // using difference between frame 0 and 1's tile index 1.
            pQAV->frames[0].tiles[3] = pQAV->frames[1].tiles[3];
            pQAV->frames[0].tiles[3].x += pQAV->frames[0].tiles[1].x - pQAV->frames[1].tiles[1].x;
            pQAV->frames[0].tiles[3].y += pQAV->frames[0].tiles[1].y - pQAV->frames[1].tiles[1].y;

            // Clone frame 3 tile index 0, 1, 2, 3 and 4 into frames 4 and 5, and adjust x/y coordinates
            // using difference between frames 4 and 3 and 5 and 4 on looped tile index.
            for (i = 4; i < 6; i++)
            {
                for (j = 0; j < 5; j++)
                {
                    pQAV->frames[i].tiles[j] = pQAV->frames[i - 1].tiles[j];
                    pQAV->frames[i].tiles[j].x += pQAV->frames[i - 1].tiles[j].x - pQAV->frames[i - 2].tiles[j].x;
                    pQAV->frames[i].tiles[j].y += pQAV->frames[i - 1].tiles[j].y - pQAV->frames[i - 2].tiles[j].y;
                }
            }
            break;
        case kQAVREMDROP:
            // REMDROP has several tile indices that require repairs to avoid needing its own interpolation callback.
            // Additionally, there are missing frames crucial to a proper interpolation experience.

            // For frame 1, move tile index 2 into 6, and 3 into 2, and disable original index of 3.
            pQAV->frames[1].tiles[6] = pQAV->frames[1].tiles[2];
            pQAV->frames[1].tiles[2] = pQAV->frames[1].tiles[3];
            pQAV->frames[1].tiles[3].picnum = -1;

            // Clone frame 3 tile index 0 and 1 into frames 4, and adjust x/y coordinates
            // using difference between frames 4 and 3 on looped tile index.
            for (j = 0; j < 2; j++)
            {
                pQAV->frames[4].tiles[j] = pQAV->frames[3].tiles[j];
                pQAV->frames[4].tiles[j].x += pQAV->frames[3].tiles[j].x - pQAV->frames[2].tiles[j].x;
                pQAV->frames[4].tiles[j].y += pQAV->frames[3].tiles[j].y - pQAV->frames[2].tiles[j].y;
            }
            break;
        case kQAVREMTHRO:
            // REMTHRO has several tile indices that require repairs.

            // For frame 1, swap tile index 2 and 3 around.
            backup = pQAV->frames[1].tiles[3];
            pQAV->frames[1].tiles[3] = pQAV->frames[1].tiles[2];
            pQAV->frames[1].tiles[2] = backup;

            // For frame 0, clone frame 1 tile index 3 and adjust x/y coordinates
            // using difference between frame 0 and 1's tile index 1.
            pQAV->frames[0].tiles[3] = pQAV->frames[1].tiles[3];
            pQAV->frames[0].tiles[3].x += pQAV->frames[0].tiles[1].x - pQAV->frames[1].tiles[1].x;
            pQAV->frames[0].tiles[3].y += pQAV->frames[0].tiles[1].y - pQAV->frames[1].tiles[1].y;

            // For frame 4, move tile index 1 into 2, and disable original index of 1.
            pQAV->frames[4].tiles[2] = pQAV->frames[4].tiles[1];
            pQAV->frames[4].tiles[1].picnum = -1;

            // For frames 5 until the end, move tile indices 0 and 1 to 2 and 3 respectively, and disable original indices.
            for (i = 5; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[3] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[1].picnum = -1;
                pQAV->frames[i].tiles[0].picnum = -1;

            }
            break;
        case kQAVFLARUP:
            // FLARUP interpolates fine, but the final frame in bringing the flaregun up is lower than the flaregun while idle.
            // Do linear interpolation from 2nd frame through to last frame, ending with coordinates of FLARIDLE.
            lastframe = pQAV->nFrames - 1;
            for (i = 1; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[0].x = xs_CRoundToInt(pQAV->frames[0].tiles[0].x - (double(pQAV->frames[0].tiles[0].x - 57) / lastframe) * i);
                pQAV->frames[i].tiles[0].y = xs_CRoundToInt(pQAV->frames[0].tiles[0].y - (double(pQAV->frames[0].tiles[0].y - -30) / lastframe) * i);
            }
            break;
        case kQAVFLARFIR2:
            // FLARFIR2 has several index swaps that require accomodating.
            // For frames 4 until end, move tile index 0 to 1 and disable original index of 0.
            for (i = 4; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[1] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0].picnum = -1;
            }

            // For frame 1, move tile indices 1 and 2 into 2 and 3, and disable original index of 1.
            pQAV->frames[1].tiles[3] = pQAV->frames[1].tiles[2];
            pQAV->frames[1].tiles[2] = pQAV->frames[1].tiles[1];
            pQAV->frames[1].tiles[1].picnum = -1;

            // For frame 0, move tile indices 0 and 1 into 2 and 4, and disable original indices.
            pQAV->frames[0].tiles[4] = pQAV->frames[0].tiles[1];
            pQAV->frames[0].tiles[2] = pQAV->frames[0].tiles[0];
            pQAV->frames[0].tiles[1].picnum = -1;
            pQAV->frames[0].tiles[0].picnum = -1;
            break;
        case kQAVFLARDOWN:
            // FLARDOWN interpolates fine, but the starting frame in bringing the flaregun down is lower than the flaregun while idle.
            // Do linear interpolation from 2nd last frame through to first frame, ending with coordinates of FLARIDLE.
            lastframe = pQAV->nFrames - 1;
            for (i = lastframe, j = 0; i >= 0; i--, j++)
            {
                pQAV->frames[j].tiles[0].x = xs_CRoundToInt(pQAV->frames[lastframe].tiles[0].x - (double(pQAV->frames[lastframe].tiles[0].x - 57) / lastframe) * i);
                pQAV->frames[j].tiles[0].y = xs_CRoundToInt(pQAV->frames[lastframe].tiles[0].y - (double(pQAV->frames[lastframe].tiles[0].y - -30) / lastframe) * i);
            }
            break;
        case kQAVFLAR2FIR:
            // FLAR2FIR has several index swaps that require accomodating and to ensure it interpolates right.

            // Handle x > 0 side first.
            // For frame 0, move tile indices 0 and 1 into 5 and 7, and disable original indices.
            pQAV->frames[0].tiles[7] = pQAV->frames[0].tiles[1];
            pQAV->frames[0].tiles[5] = pQAV->frames[0].tiles[0];
            pQAV->frames[0].tiles[1].picnum = -1;
            pQAV->frames[0].tiles[0].picnum = -1;

            // For frame 1, move tile indices 1 and 2 into 5 and 6, and disable original indices of 1 and 2.
            for (i = 1; i < 3; i++)
            {
                pQAV->frames[1].tiles[i + 4] = pQAV->frames[1].tiles[i];
                pQAV->frames[1].tiles[i].picnum = -1;
            }

            // For frames 2 and 3, move tile index 1 into 7, and disable original index of 1.
            for (i = 2; i < 4; i++)
            {
                pQAV->frames[i].tiles[7] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }

            // For frames 4 until end, move tile index 0 into 7, and disable original index of 0.
            for (i = 4; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[7] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0].picnum = -1;
            }

            // Handle x < 0 now.
            // For frame 0, move tile index 2 into 4, and disable the original index of 2.
            pQAV->frames[0].tiles[4] = pQAV->frames[0].tiles[2];
            pQAV->frames[0].tiles[2].picnum = -1;

            // For frame 1, move tile index 3 into 4, and disable the original index of 3.
            pQAV->frames[1].tiles[4] = pQAV->frames[1].tiles[3];
            pQAV->frames[1].tiles[3].picnum = -1;

            // For frames 2 and 3, move tile index 2 into 4, and disable the original index of 2.
            for (i = 2; i < 4; i++)
            {
                pQAV->frames[i].tiles[4] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2].picnum = -1;
            }

            // For frame 3, move tile index 3 into 6, and disable the original index of 3.
            pQAV->frames[3].tiles[6] = pQAV->frames[3].tiles[3];
            pQAV->frames[3].tiles[3].picnum = -1;

            // For frame 4, move tile indices 2 and 3 into 4 and 5, and disable the original indices of 2 and 3.
            for (i = 2; i < 4; i++)
            {
                pQAV->frames[4].tiles[i + 2] = pQAV->frames[4].tiles[i];
                pQAV->frames[4].tiles[i].picnum = -1;
            }

            // For frames 5 and 6, move tile index 2 into 6, and disable the original index of 2.
            for (i = 5; i < 7; i++)
            {
                pQAV->frames[i].tiles[6] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2].picnum = -1;
            }

            // For frames 7 until end, move tile index 1 into 6, and disable original index of 1.
            for (i = 7; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[6] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }
            break;
        case kQAVSHOTUP:
            // SHOTUP is missing tiles for the first two frames.
            // Clone from 3rd frame and amend x/y coordinates for tile indices 1 and 2 on frames 0 and 1.
            for (i = 0; i < 2; i++)
            {
                for (j = 1; j < 3; j++)
                {
                    pQAV->frames[i].tiles[j] = pQAV->frames[2].tiles[j];
                    pQAV->frames[i].tiles[j].x += pQAV->frames[i].tiles[0].x - pQAV->frames[2].tiles[0].x;
                    pQAV->frames[i].tiles[j].y += pQAV->frames[i].tiles[0].y - pQAV->frames[2].tiles[0].y;
                }
            }
            break;
        case kQAV2SHOTF2:
            // 2SHOTF2 has tiles 2630 and 2631 applied when it doesn't need to,
            // and is missing a preceding frame of tile 2632 which the non-akimbo version has.

            // Patch left and right side respectively.
            pQAV->frames[0].tiles[6] = pQAV->frames[1].tiles[6];
            pQAV->frames[0].tiles[6].x -= pQAV->frames[1].tiles[5].x - pQAV->frames[0].tiles[5].x;
            pQAV->frames[0].tiles[6].y -= pQAV->frames[1].tiles[5].y - pQAV->frames[0].tiles[5].y;
            pQAV->frames[3].tiles[3] = pQAV->frames[4].tiles[3];
            pQAV->frames[3].tiles[3].x -= pQAV->frames[4].tiles[2].x - pQAV->frames[3].tiles[2].x;
            pQAV->frames[3].tiles[3].y -= pQAV->frames[4].tiles[2].y - pQAV->frames[3].tiles[2].y;

            // Stabilise frame 2 tile 2 by using x/y coordindates from next frame.
            pQAV->frames[2].tiles[2].x = pQAV->frames[3].tiles[2].x;
            pQAV->frames[2].tiles[2].y = pQAV->frames[3].tiles[2].y;

            // Disable frame 0 tile 7.
            pQAV->frames[0].tiles[7].picnum = -1;
            break;
        case kQAV2SHOTFIR:
            // 2SHOTFIR has the issues of both SHOTUP and 2SHOTF2, fix both issues.

            // Fix missing tiles for 2630 and 2631 and amend x/y coordinates for 2630 and 2631 for right and left side respectively.
            for (i = 8; i < 11; i++)
            {
                // Use difference from tile 2, it's the same for left and right side.
                for (j = 3; j < 5; j++)
                {
                    pQAV->frames[i].tiles[j] = pQAV->frames[11].tiles[j];
                    pQAV->frames[i].tiles[j].x += pQAV->frames[i].tiles[2].x - pQAV->frames[11].tiles[2].x;
                    pQAV->frames[i].tiles[j].y += pQAV->frames[i].tiles[2].y - pQAV->frames[11].tiles[2].y;
                }
                for (j = 6; j < 8; j++)
                {
                    pQAV->frames[i].tiles[j] = pQAV->frames[11].tiles[j];
                    pQAV->frames[i].tiles[j].x -= pQAV->frames[i].tiles[2].x - pQAV->frames[11].tiles[2].x;
                    pQAV->frames[i].tiles[j].y += pQAV->frames[i].tiles[2].y - pQAV->frames[11].tiles[2].y;
                }
            }

            // Fix missing tiles for 2632 and patch coordinates on right and left side respectively.
            for (i = 3; i < 7; i += 3)
            {
                pQAV->frames[0].tiles[i] = pQAV->frames[1].tiles[i];
                pQAV->frames[0].tiles[i].x -= pQAV->frames[1].tiles[i - 1].x - pQAV->frames[0].tiles[i - 1].x;
                pQAV->frames[0].tiles[i].y -= pQAV->frames[1].tiles[i - 1].y - pQAV->frames[0].tiles[i - 1].y;
            }

            // Disable frame 0 tile 4 and tile 7.
            pQAV->frames[0].tiles[4].picnum = -1;
            pQAV->frames[0].tiles[7].picnum = -1;
            break;
        case kQAVSGUNUP:
            // SGUNUP has a missing frame crucial to proper interpolation experience, so add it back
            // in and adjust x/y coordinates using difference between frame 0 and 1's tile index 0.
            pQAV->frames[0].tiles[1] = pQAV->frames[1].tiles[1];
            pQAV->frames[0].tiles[1].x -= pQAV->frames[1].tiles[0].x - pQAV->frames[0].tiles[0].x;
            pQAV->frames[0].tiles[1].y -= pQAV->frames[1].tiles[0].y - pQAV->frames[0].tiles[0].y;
            break;
        case kQAVSGUNIDL1:
            // SGUNIDL1 has overlay 3232 permanently applied which is at odds with the weapon rising,
            // and the other idling QAV. Disable it entirely.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[1].picnum = -1;
            }
            break;
        case kQAVSGUNFIR1:
        {
            // SGUNFIR1's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[8] = { 9301, 9302, 9303, 9304, 9300, 9301, 9302, 3227 };

            // Loop through each frame to remove overlay and replace use of 3227 with that from tilearray.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[0] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = pQAV->frames[i].tiles[3];
                pQAV->frames[i].tiles[0].picnum = tilearray[i];
                pQAV->frames[i].tiles[2].picnum = -1;
                pQAV->frames[i].tiles[3].picnum = -1;

            }
            break;
        }
        case kQAVSGUNFIR4:
        {
            // SGUNFIR4's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[46] = { 9304, 9304, 9300, 9300, 9301, 9301, 9302, 9302, 9303, 9303, 9304, 9300, 9301, 9302, 9303, 9304, 9300, 9301, 9302, 9303, 9304, 9300, 9301, 9302, 9303, 9304, 9300, 9301, 9302, 9303, 9300, 9302, 9304, 9301, 9303, 9300, 9302, 9304, 9301, 9303, 9301, 9304, 9302, 9300, 9303, 9301 };

            // Loop through each frame to remove overlay and replace use of 3227 with that from tilearray.
            for (i = 0; i < 46; i++)
            {
                pQAV->frames[i].tiles[0].picnum = tilearray[i];
                pQAV->frames[i].tiles[1].picnum = -1;
            }

            // Swap frame 46 index 0 and 1.
            backup = pQAV->frames[46].tiles[0];
            pQAV->frames[46].tiles[0] = pQAV->frames[46].tiles[1];
            pQAV->frames[46].tiles[1] = backup;
            break;
        }
        case kQAVSGUNPRE:
            // SGUNPRE's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.

            // For the two last frames, change tile index 0 picnum from 3227 to 9300.
            for (i = pQAV->nFrames - 2; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[0].picnum = 9300;
                pQAV->frames[i].tiles[2].picnum = -1;
            }
            break;
        case kQAVSGUNPOST:
        {
            // SGUNPOST's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[8] = { 9301, 9301, 9302, 9302, 9303, 9303, 9304, 9304 };

            // Loop through each frame to remove overlay and replace use of 3227 with that from tilearray.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[1].picnum = tilearray[i];
                pQAV->frames[i].tiles[2].picnum = -1;

            }
            break;
        }
        case kQAV2SGUNUP:
            // 2SGUNUP has a missing two frame tiles crucial to proper interpolation experience, so add them back
            // in and adjust x/y coordinates using difference between frame 0 and 1's tile index 0 and 2.
            for (i = 1; i < 4; i += 2)
            {
                pQAV->frames[0].tiles[i] = pQAV->frames[1].tiles[i];
                pQAV->frames[0].tiles[i].x -= pQAV->frames[1].tiles[i-1].x - pQAV->frames[0].tiles[i-1].x;
                pQAV->frames[0].tiles[i].y -= pQAV->frames[1].tiles[i-1].y - pQAV->frames[0].tiles[i-1].y;
            }
            
            // Set frame 0 tile 3 picnum to 3311.
            pQAV->frames[0].tiles[3].picnum = 3311;
            break;
        case kQAV2SGUNFIR:
        {
            // 2SGUNFIR's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[2][8] = {
                {
                    9306, 9307, 9308, 9309, 9305, 9306, 9307, 3240
                },
                {
                    3240, 9307, 9306, 9305, 9309, 9308, 9307, 9306
                }
            };

            // Loop through each frame to remove overlay and replace use of 3240 with that from tilearray and disable overlays.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[0].picnum = tilearray[0][i];
                pQAV->frames[i].tiles[4].picnum = -1;

                pQAV->frames[i].tiles[2].picnum = tilearray[1][i];
                pQAV->frames[i].tiles[5].picnum = -1;
            }
            break;
        }
        case kQAV2SGUNALT:
        {
            // 2SGUNALT's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[2][30] = {
                {
                    9309, 9305, 9306, 9307, 9308, 9309, 9305, 9306, 9307, 9308, 9309, 9305, 9306, 9307, 9308, 9309, 9305, 9306, 9307, 9308, 9309, 9305, 9306, 9307, 9308, 9309, 9305, 9306, 9307, 9308
                },
                {
                    9308, 9307, 9306, 9305, 9309, 9308, 9307, 9306, 9305, 9309, 9308, 9307, 9306, 9305, 9309, 9308, 9307, 9306, 9305, 9309, 9308, 9307, 9306, 9305, 9309, 9308, 9307, 9306, 9305, 9309
                }
            };

            // Loop through first 30 frames to remove overlay and replace use of 3240 with that from tilearray and disable overlays.
            for (i = 0; i < 30; i++)
            {
                pQAV->frames[i].tiles[0].picnum = tilearray[0][i];
                pQAV->frames[i].tiles[4].picnum = -1;

                pQAV->frames[i].tiles[2].picnum = tilearray[1][i];
                pQAV->frames[i].tiles[5].picnum = -1;
            }

            // Disable tiles 1, 3 and 6 on frame 29.
            pQAV->frames[29].tiles[1].picnum = pQAV->frames[29].tiles[3].picnum = pQAV->frames[29].tiles[6].picnum = -1;
            break;
        }
        case kQAV2SGUNPRE:
        {
            // 2SGUNPRE's overlay sizes vary from tile to tile and don't interpolate properly.
            // Use repaired tiles from Phredreeke where the overlays are baked in.
            constexpr int tilearray[2][30] = {
                {
                    9305, 9306, 9307, 9308
                },
                {
                    9307, 9306, 9305, 9309
                }
            };

            // Loop through all frames to remove overlay and replace use of 3240 with that from tilearray and disable overlays.
            for (i = 0; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[0].picnum = tilearray[0][i];
                pQAV->frames[i].tiles[4].picnum = -1;

                pQAV->frames[i].tiles[2].picnum = tilearray[1][i];
                pQAV->frames[i].tiles[5].picnum = -1;
            }
            break;
        }
        case kQAVNAPUP:
            // This QAV is missing picnum 2351 for the first two frames. Add it in to smooth interpolation.
            for (i = 0; i < 2; i++)
            {
                pQAV->frames[i].tiles[1] = pQAV->frames[2].tiles[1];
                pQAV->frames[i].tiles[1].x += pQAV->frames[i].tiles[0].x - pQAV->frames[2].tiles[0].x;
                pQAV->frames[i].tiles[1].y += pQAV->frames[i].tiles[0].y - pQAV->frames[2].tiles[0].y;
                pQAV->frames[i].tiles[1].picnum = 2351;
            }
            break;
        case kQAVVDUP:
            // VDUP requires tile indices on the last frame to be swapped around.
            backup = pQAV->frames[pQAV->nFrames-1].tiles[0];
            pQAV->frames[pQAV->nFrames-1].tiles[0] = pQAV->frames[pQAV->nFrames - 1].tiles[1];
            pQAV->frames[pQAV->nFrames-1].tiles[1] = backup;
            break;
        case kQAVVDIDLE2:
        {
            // VDIDLE2 requires several tile indices to be swapped around to fix interpolation.
            // For frames 5 and 7, move tile index 2 into 0 and disable the original index of 0.
            for (i = 5; i < 8; i += 2)
            {
                pQAV->frames[i].tiles[0] = pQAV->frames[i].tiles[2];
                pQAV->frames[i].tiles[2].picnum = -1;
            }

            // For frames 2-3, swap tile indices 0 and 1 around.
            for (i = 2; i < 4; i++)
            {
                backup = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = backup;
            }

            // For frames 9 til the end, swap tile indices 0 and 1 around.
            for (i = 9; i < pQAV->nFrames; i++)
            {
                backup = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = backup;
            }

            // Set frame 1 tile 0 x/y coordinates to that of values between frames 0 and 2.
            pQAV->frames[1].tiles[0].x = -29;
            pQAV->frames[1].tiles[0].y = -12;

            // Set frame 1 tile 1 y coordinates to that of values between frames 0 and 2.
            pQAV->frames[1].tiles[1].y = -45;

            // Set frame 3 tile 1 y coordinates to that of values between frames 2 and 4.
            pQAV->frames[3].tiles[1].y = -46;

            // Set frame 5 tile 1 y coordinates to that of values between frames 4 and 6.
            pQAV->frames[5].tiles[1].y = -42;

            // Set frame 7 tile 1 y coordinates to that of values between frames 6 and 8.
            pQAV->frames[7].tiles[1].y = -41;

            // Set frame 10 tile 1 y coordinates to that of values between frames 9 and 11.
            pQAV->frames[10].tiles[1].y = -45;

            // Smooth out tile coordinates between high and low point for tile index 1 x across all frames. High point is frame 0/12, low point is 6.
            for (i = 1, j = (pQAV->nFrames - 2); i < 6, j > 6; i++, j--)
            {
                pQAV->frames[i].tiles[1].x = pQAV->frames[j].tiles[1].x = xs_CRoundToInt(pQAV->frames[0].tiles[1].x - (double(pQAV->frames[0].tiles[1].x - pQAV->frames[6].tiles[1].x) / 6) * i);
            }
            break;
        }
        case kQAVVDFIRE1:
        case kQAVVDFIRE2:
            // VDFIRE1 and VDFIRE2 requires several index swaps to repair interpolations.
            // For frame 0, move tile index 1 to 2, and disable original index of 1.
            pQAV->frames[0].tiles[2] = pQAV->frames[0].tiles[1];
            pQAV->frames[0].tiles[1].picnum = -1;

            // For frame 1, move tile index 0 to 2 and 1 to 0, and disable original index of 1.
            pQAV->frames[0].tiles[2] = pQAV->frames[0].tiles[0];
            pQAV->frames[0].tiles[0] = pQAV->frames[0].tiles[1];
            pQAV->frames[0].tiles[1].picnum = -1;

            // For frame 7, swap tile indices 1 to 2.
            backup = pQAV->frames[7].tiles[2];
            pQAV->frames[7].tiles[2] = pQAV->frames[7].tiles[1];
            pQAV->frames[7].tiles[1] = backup;

            // For frame 8-9, move tile index 1 to 2, and disable original index of 1.
            for (i = 8; i < 10; i++)
            {
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }

            // For frames 10 till end, move tile index 0 to 2 and 1 to 0, and disable original index of 1.
            for (i = 10; i < pQAV->nFrames; i++)
            {
                pQAV->frames[i].tiles[2] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0] = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1].picnum = -1;
            }
            break;
        case kQAVVDFIRE3:
            // VDFIRE3 requires several index swaps to repair interpolations.
            // For frame 1, swap tile indices 0 and 1.
            backup = pQAV->frames[1].tiles[1];
            pQAV->frames[1].tiles[1] = pQAV->frames[1].tiles[0];
            pQAV->frames[1].tiles[0] = backup;

            // For frames 13 till end, swap tile indices 0 and 1.
            for (i = 13; i < pQAV->nFrames; i++)
            {
                backup = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0] = backup;
            }
            break;
        case kQAVVDFIRE4:
        case kQAVVDFIRE5:
            // VDFIRE4 and requires several index swaps to repair interpolations.
            // For frame 1, swap tile indices 0 and 1.
            backup = pQAV->frames[1].tiles[1];
            pQAV->frames[1].tiles[1] = pQAV->frames[1].tiles[0];
            pQAV->frames[1].tiles[0] = backup;

            // For the last two frames, swap tile indices 0 and 1.
            for (i = (pQAV->nFrames - 2); i < pQAV->nFrames; i++)
            {
                backup = pQAV->frames[i].tiles[1];
                pQAV->frames[i].tiles[1] = pQAV->frames[i].tiles[0];
                pQAV->frames[i].tiles[0] = backup;
            }
            break;
        default:
            return;
    }
}


// This is to eliminate a huge design issue in NBlood that was apparently copied verbatim from the DOS-Version.
// Sequences were cached in the resource and directly returned from there in writable form, with byte swapping directly performed in the cache on Big Endian systems.
// To avoid such unsafe operations this caches the read data separately.
extern FMemArena seqcache; // Use the same storage as the SEQs.
static TMap<int, QAV*> qavcache;
QAV* getQAV(int res_id)
{
    auto p = qavcache.CheckKey(res_id);
    if (p != nullptr) return *p;

    int index = fileSystem.FindResource(res_id, "QAV");
    if (index < 0)
    {
        return nullptr;
    }
    auto fr = fileSystem.OpenFileReader(index);

    // Start reading QAV for nFrames, skipping padded data.
    for (int i = 0; i < 8; i++) fr.ReadUInt8();
    int nFrames = fr.ReadInt32();
    auto qavdata = (QAV*)seqcache.Alloc(sizeof(QAV) + ((nFrames - 1) * sizeof(FRAMEINFO)));

    // Write out QAV data.
    qavdata->nFrames = nFrames;
    qavdata->ticksPerFrame = fr.ReadInt32();
    qavdata->duration = fr.ReadInt32();
    qavdata->x = fr.ReadInt32();
    qavdata->y = fr.ReadInt32();
    qavdata->nSprite = fr.ReadInt32();
    for (int i = 0; i < 4; i++) fr.ReadUInt8();

    // Read FRAMEINFO data.
    for (int i = 0; i < qavdata->nFrames; i++)
    {
        qavdata->frames[i].nCallbackId = fr.ReadInt32();

        // Read SOUNDINFO data.
        qavdata->frames[i].sound.sound = fr.ReadInt32();
        qavdata->frames[i].sound.priority = fr.ReadUInt8();
        qavdata->frames[i].sound.sndFlags = fr.ReadUInt8();
        qavdata->frames[i].sound.sndRange = fr.ReadUInt8();
        for (int i = 0; i < 1; i++) fr.ReadUInt8();

        // Read TILE_FRAME data.
        for (int j = 0; j < 8; j++)
        {
            qavdata->frames[i].tiles[j].picnum = fr.ReadInt32();
            qavdata->frames[i].tiles[j].x = fr.ReadInt32();
            qavdata->frames[i].tiles[j].y = fr.ReadInt32();
            qavdata->frames[i].tiles[j].z = fr.ReadInt32();
            qavdata->frames[i].tiles[j].stat = fr.ReadInt32();
            qavdata->frames[i].tiles[j].shade = fr.ReadInt8();
            qavdata->frames[i].tiles[j].palnum = fr.ReadUInt8();
            qavdata->frames[i].tiles[j].angle = fr.ReadUInt16();
        }
    }

    // Write out additions.
    qavdata->res_id = res_id;
    qavdata->ticrate = 120. / qavdata->ticksPerFrame;

    // Repair tile data here for now until we export all repaired QAVs.
    qavRepairTileData(qavdata);

    // Build QAVInterpProps struct here for now until we get DEF loading going.
    qavBuildInterpProps(qavdata);

    qavcache.Insert(res_id, qavdata);
    return qavdata;
}


END_BLD_NS
