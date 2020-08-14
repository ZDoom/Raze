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
#include "qav.h"
#include "sound.h"
#include "v_draw.h"
#include "glbackend/glbackend.h"

BEGIN_BLD_NS

#define kMaxClients 64
static void (*clientCallback[kMaxClients])(int, void *);
static int nClients;


int qavRegisterClient(void(*pClient)(int, void *))
{
    dassert(nClients < kMaxClients);
    clientCallback[nClients] = pClient;

    return nClients++;
}

void DrawFrame(double x, double y, TILE_FRAME *pTile, int stat, int shade, int palnum, bool to3dview)
{
    stat |= pTile->stat;
	x += pTile->x;
	y += pTile->y;

    if (!to3dview)
    {
		auto tex = tileGetTexture(pTile->picnum);
		double scale = pTile->z/65536.;
		double angle = pTile->angle * (360./2048);
		int renderstyle = (stat & RS_NOMASK)? STYLE_Normal : STYLE_Translucent;
		double alpha = (stat & RS_TRANS1)? glblend[0].def[!!(stat & RS_TRANS2)].alpha : 1.;
		int pin = (stat & kQavOrientationLeft)? -1 : (stat & RS_ALIGN_R)? 1:0;
		if (palnum <= 0) palnum = pTile->palnum;
		auto translation = TRANSLATION(Translation_Remap, palnum);
		bool topleft = !!(stat & RS_TOPLEFT);

		bool xflip = !!(stat & 0x100); // repurposed flag
		bool yflip = !!(stat & RS_YFLIP);
		auto color = shadeToLight(pTile->shade + shade);

		DrawTexture(twod, tex, x, y, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Rotate, angle, DTA_LegacyRenderStyle, renderstyle, DTA_Alpha, alpha, DTA_Pin, pin, DTA_TranslationIndex, translation,
					DTA_TopLeft, topleft, DTA_CenterOffsetRel, !topleft, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_FlipOffsets, true, DTA_Color, color,
					DTA_FlipX, xflip, DTA_FlipY, yflip, TAG_DONE);
    }
    else
    {
		if (stat & RS_YFLIP) stat |= RS_YFLIPHUD;
		stat &= ~RS_YFLIP;
		if (stat & 0x100) stat |= RS_XFLIPHUD;
		if ((stat & kQavOrientationLeft)) stat |= RS_ALIGN_L;

		hud_drawsprite(x, y, pTile->z, pTile->angle, pTile->picnum, shade, palnum, stat);
    }
}

void QAV::Draw(double x, double y, int ticks, int stat, int shade, int palnum, bool to3dview)
{
    dassert(ticksPerFrame > 0);
    int nFrame = ticks / ticksPerFrame;
    dassert(nFrame >= 0 && nFrame < nFrames);
    FRAMEINFO *pFrame = &frames[nFrame];
    for (int i = 0; i < 8; i++)
    {
        if (pFrame->tiles[i].picnum > 0)
            DrawFrame(x, y, &pFrame->tiles[i], stat, shade, palnum, to3dview);
    }
}

void QAV::Draw(int ticks, int stat, int shade, int palnum, bool to3dview)
{
    Draw(x, y, ticks, stat, shade, palnum, to3dview);
}


void QAV::Play(int start, int end, int nCallback, void *pData)
{
    dassert(ticksPerFrame > 0);
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
                
                if (nSprite == -1) PlaySound(sound);
                else PlaySound3D(&sprite[nSprite], sound, 16+pSound->priority, 6);
            }
            
            if (pFrame->nCallbackId > 0 && nCallback != -1) {
                clientCallback[nCallback](pFrame->nCallbackId, pData);
            }
        }
    }
}

void QAV::Preload(void)
{
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (frames[i].tiles[j].picnum >= 0)
                tilePreloadTile(frames[i].tiles[j].picnum);
        }
    }
}

void QAV::Precache(void)
{
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (frames[i].tiles[j].picnum >= 0)
                tilePrecacheTile(frames[i].tiles[j].picnum, 0);
        }
    }
}


void ByteSwapQAV(void* p)
{
#if B_BIG_ENDIAN == 1
    QAV* qav = (QAV*)p;
    qav->nFrames = LittleLong(qav->nFrames);
    qav->ticksPerFrame = LittleLong(qav->ticksPerFrame);
    qav->at10 = LittleLong(qav->at10);
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
static TMap<int, QAV*> sequences;
QAV* getQAV(int res_id)
{
    auto p = sequences.CheckKey(res_id);
    if (p != nullptr) return *p;

    int index = fileSystem.FindResource(res_id, "QAV");
    if (index < 0)
    {
        return nullptr;
    }
    auto fr = fileSystem.OpenFileReader(index);
    auto qavdata = (QAV*)seqcache.Alloc(fr.GetLength());
    fr.Read(qavdata, fr.GetLength());
    sequences.Insert(res_id, qavdata);
    ByteSwapQAV(qavdata);
    return qavdata;
}


END_BLD_NS
