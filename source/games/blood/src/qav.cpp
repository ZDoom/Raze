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

void QAV::Draw(double x, double y, int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio, bool const menudrip)
{
    assert(ticksPerFrame > 0);

    int nFrame = ticks / ticksPerFrame;
    assert(nFrame >= 0 && nFrame < nFrames);
    FRAMEINFO *thisFrame = &frames[nFrame];

    if ((nFrame == (nFrames - 1)) && !lastframetic)
    {
        lastframetic = ticks;
    }
    else if (lastframetic > ticks)
    {
        lastframetic = 0;
    }

    int oFrame = nFrame == 0 || (lastframetic && ticks > lastframetic) ? nFrame : nFrame - 1;
    assert(oFrame >= 0 && oFrame < nFrames);
    FRAMEINFO *prevFrame = &frames[oFrame];

    auto drawTile = [&](TILE_FRAME *thisTile, TILE_FRAME *prevTile, bool const interpolate = true)
    {
        double tileX = x;
        double tileY = y;
        double tileZ;
        double tileA;

        if (cl_bloodhudinterp && prevTile && cl_hudinterpolation && (nFrames > 1) && (nFrame != oFrame) && (smoothratio != MaxSmoothRatio) && interpolate)
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
    };

    for (int i = 0; i < 8; i++)
    {
        TILE_FRAME *thisTile = &thisFrame->tiles[i];
        TILE_FRAME *prevTile = nullptr;

        if (thisTile->picnum > 0)
        {
            // Menu's blood drip requires special treatment.
            if (menudrip)
            {
                if (i != 0)
                {
                    // Find previous frame by iterating all previous frame's tiles and match on the consistent x coordinate.
                    // Tile indices can change between frames for no reason, we need to accomodate that.
                    for (int j = 0; j < 8; j++) if (thisTile->x == prevFrame->tiles[j].x)
                    {
                        prevTile = &prevFrame->tiles[j];
                        break;
                    }

                    drawTile(thisTile, prevTile, true);
                }
                else
                {
                    // First index is always the dripping bar at the top.
                    drawTile(thisTile, prevTile, false);
                }
            }
            else
            {
                prevTile = &prevFrame->tiles[i];
                drawTile(thisTile, prevTile, thisTile->picnum == prevTile->picnum);
            }
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


void ByteSwapQAV(void* p)
{
#if B_BIG_ENDIAN == 1
    QAV* qav = (QAV*)p;
    qav->nFrames = LittleLong(qav->nFrames);
    qav->ticksPerFrame = LittleLong(qav->ticksPerFrame);
    qav->version = LittleLong(qav->version);
    qav->x = LittleLong(qav->x);
    qav->y = LittleLong(qav->y);
    qav->nSprite = LittleLong(qav->nSprite);
    for (int i = 0; i < qav->nFrames; i++)
    {
        FRAMEINFO* pFrame = &qav->frames[i];
        SOUNDINFO* pSound = &pFrame->sound;
        pFrame->nCallbackId = LittleLong(pFrame->nCallbackId);
        pSound->sound = LittleLong(pSound->sound);
        for (int j = 0; j < 8; j++)
        {
            TILE_FRAME* pTile = &pFrame->tiles[j];
            pTile->picnum = LittleLong(pTile->picnum);
            pTile->x = LittleLong(pTile->x);
            pTile->y = LittleLong(pTile->y);
            pTile->z = LittleLong(pTile->z);
            pTile->stat = LittleLong(pTile->stat);
            pTile->angle = LittleShort(pTile->angle);
        }
    }
#endif
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
    auto qavdata = (QAV*)seqcache.Alloc(fr.GetLength());
    fr.Read(qavdata, fr.GetLength());
    qavcache.Insert(res_id, qavdata);
    ByteSwapQAV(qavdata);
    return qavdata;
}


END_BLD_NS
