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
#include "compat.h"
#include "common_game.h"
#include "qav.h"
#include "sound.h"

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

void DrawFrame(int x, int y, TILE_FRAME *pTile, int stat, int shade, int palnum)
{
    stat |= pTile->stat;
    int angle = pTile->angle;
    if (stat & 0x100)
    {
        angle = (angle+1024)&2047;
        stat &= ~0x100;
        stat ^= 0x4;
    }
    if (stat & kQavOrientationLeft)
    {
        stat &= ~kQavOrientationLeft;
        stat |= 256;
    }
    if (palnum <= 0)
        palnum = pTile->palnum;
    rotatesprite((x + pTile->x) << 16, (y + pTile->y) << 16, pTile->z, angle,
                 pTile->picnum, ClipRange(pTile->shade + shade, -128, 127), palnum, stat,
                 windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}

void QAV::Draw(int ticks, int stat, int shade, int palnum)
{
    dassert(ticksPerFrame > 0);
    int nFrame = ticks / ticksPerFrame;
    dassert(nFrame >= 0 && nFrame < nFrames);
    FRAMEINFO *pFrame = &frames[nFrame];
    for (int i = 0; i < 8; i++)
    {
        if (pFrame->tiles[i].picnum > 0)
            DrawFrame(x, y, &pFrame->tiles[i], stat, shade, palnum);
    }
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
    qav->nFrames = B_LITTLE32(qav->nFrames);
    qav->ticksPerFrame = B_LITTLE32(qav->ticksPerFrame);
    qav->at10 = B_LITTLE32(qav->at10);
    qav->x = B_LITTLE32(qav->x);
    qav->y = B_LITTLE32(qav->y);
    qav->nSprite = B_LITTLE32(qav->nSprite);
    for (int i = 0; i < qav->nFrames; i++)
    {
        FRAMEINFO* pFrame = &qav->frames[i];
        SOUNDINFO* pSound = &pFrame->sound;
        pFrame->nCallbackId = B_LITTLE32(pFrame->nCallbackId);
        pSound->sound = B_LITTLE32(pSound->sound);
        for (int j = 0; j < 8; j++)
        {
            TILE_FRAME* pTile = &pFrame->tiles[j];
            pTile->picnum = B_LITTLE32(pTile->picnum);
            pTile->x = B_LITTLE32(pTile->x);
            pTile->y = B_LITTLE32(pTile->y);
            pTile->z = B_LITTLE32(pTile->z);
            pTile->stat = B_LITTLE32(pTile->stat);
            pTile->angle = B_LITTLE16(pTile->angle);
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
