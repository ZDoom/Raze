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
