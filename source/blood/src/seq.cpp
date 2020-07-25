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

#include <string.h>
#include "build.h"
#include "common_game.h"

#include "blood.h"
#include "db.h"
#include "eventq.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "sound.h"
#include "seq.h"
#include "gameutil.h"
#include "actor.h"
#include "view.h"
#include "raze_sound.h"

BEGIN_BLD_NS

#define kMaxClients 256
#define kMaxSequences 1024

static ACTIVE activeList[kMaxSequences];
static int activeCount = 0;
static int nClients = 0;
static void(*clientCallback[kMaxClients])(int, int);

int seqRegisterClient(void(*pClient)(int, int))
{
    dassert(nClients < kMaxClients);
    clientCallback[nClients] = pClient;
    return nClients++;
}

void Seq::Preload(void)
{
    if (memcmp(signature, "SEQ\x1a", 4) != 0)
        ThrowError("Invalid sequence");
    if ((version & 0xff00) != 0x300)
        ThrowError("Obsolete sequence version");
    for (int i = 0; i < nFrames; i++)
        tilePreloadTile(seqGetTile(&frames[i]));
}

void Seq::Precache(void)
{
    if (memcmp(signature, "SEQ\x1a", 4) != 0)
        ThrowError("Invalid sequence");
    if ((version & 0xff00) != 0x300)
        ThrowError("Obsolete sequence version");
    for (int i = 0; i < nFrames; i++)
        tilePrecacheTile(seqGetTile(&frames[i]));
}

void seqPrecacheId(int id)
{
    auto pSeq = getSequence(id);
    if (pSeq) pSeq->Precache();
}

SEQINST siWall[kMaxXWalls];
SEQINST siCeiling[kMaxXSectors];
SEQINST siFloor[kMaxXSectors];
SEQINST siSprite[kMaxXSprites];
SEQINST siMasked[kMaxXWalls];

void UpdateSprite(int nXSprite, SEQFRAME *pFrame)
{
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    int nSprite = xsprite[nXSprite].reference;
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->extra == nXSprite);
    if (pSprite->flags & 2)
    {
        if (tilesiz[pSprite->picnum].y != tilesiz[seqGetTile(pFrame)].y || tileTopOffset(pSprite->picnum) != tileTopOffset(seqGetTile(pFrame))
            || (pFrame->at3_0 && pFrame->at3_0 != pSprite->yrepeat))
            pSprite->flags |= 4;
    }
    pSprite->picnum = seqGetTile(pFrame);
    if (pFrame->at5_0)
        pSprite->pal = pFrame->at5_0;
    pSprite->shade = pFrame->at4_0;
    
    int scale = xsprite[nXSprite].scale; // SEQ size scaling
    if (pFrame->at2_0) {
        if (scale) pSprite->xrepeat = ClipRange(mulscale8(pFrame->at2_0, scale), 0, 255);
        else pSprite->xrepeat = pFrame->at2_0;
    }

    if (pFrame->at3_0) {
        if (scale) pSprite->yrepeat = ClipRange(mulscale8(pFrame->at3_0, scale), 0, 255);
        else pSprite->yrepeat = pFrame->at3_0;
    }

    if (pFrame->at1_4)
        pSprite->cstat |= 2;
    else
        pSprite->cstat &= ~2;
    if (pFrame->at1_5)
        pSprite->cstat |= 512;
    else
        pSprite->cstat &= ~512;
    if (pFrame->at1_6)
        pSprite->cstat |= 1;
    else
        pSprite->cstat &= ~1;
    if (pFrame->at1_7)
        pSprite->cstat |= 256;
    else
        pSprite->cstat &= ~256;
    if (pFrame->at6_2)
        pSprite->cstat |= 32768;
    else
        pSprite->cstat &= (unsigned short)~32768;
    if (pFrame->at6_0)
        pSprite->cstat |= 4096;
    else
        pSprite->cstat &= ~4096;
    if (pFrame->at5_6)
        pSprite->flags |= 256;
    else
        pSprite->flags &= ~256;
    if (pFrame->at5_7)
        pSprite->flags |= 8;
    else
        pSprite->flags &= ~8;
    if (pFrame->at6_3)
        pSprite->flags |= 1024;
    else
        pSprite->flags &= ~1024;
    if (pFrame->at6_4)
        pSprite->flags |= 2048;
    else
        pSprite->flags &= ~2048;
}

void UpdateWall(int nXWall, SEQFRAME *pFrame)
{
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    int nWall = xwall[nXWall].reference;
    dassert(nWall >= 0 && nWall < kMaxWalls);
    walltype *pWall = &wall[nWall];
    dassert(pWall->extra == nXWall);
    pWall->picnum = seqGetTile(pFrame);
    if (pFrame->at5_0)
        pWall->pal = pFrame->at5_0;
    if (pFrame->at1_4)
        pWall->cstat |= 128;
    else
        pWall->cstat &= ~128;
    if (pFrame->at1_5)
        pWall->cstat |= 512;
    else
        pWall->cstat &= ~512;
    if (pFrame->at1_6)
        pWall->cstat |= 1;
    else
        pWall->cstat &= ~1;
    if (pFrame->at1_7)
        pWall->cstat |= 64;
    else
        pWall->cstat &= ~64;
}

void UpdateMasked(int nXWall, SEQFRAME *pFrame)
{
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    int nWall = xwall[nXWall].reference;
    dassert(nWall >= 0 && nWall < kMaxWalls);
    walltype *pWall = &wall[nWall];
    dassert(pWall->extra == nXWall);
    dassert(pWall->nextwall >= 0);
    walltype *pWallNext = &wall[pWall->nextwall];
    pWall->overpicnum = pWallNext->overpicnum = seqGetTile(pFrame);
    if (pFrame->at5_0)
        pWall->pal = pWallNext->pal = pFrame->at5_0;
    if (pFrame->at1_4)
    {
        pWall->cstat |= 128;
        pWallNext->cstat |= 128;
    }
    else
    {
        pWall->cstat &= ~128;
        pWallNext->cstat &= ~128;
    }
    if (pFrame->at1_5)
    {
        pWall->cstat |= 512;
        pWallNext->cstat |= 512;
    }
    else
    {
        pWall->cstat &= ~512;
        pWallNext->cstat &= ~512;
    }
    if (pFrame->at1_6)
    {
        pWall->cstat |= 1;
        pWallNext->cstat |= 1;
    }
    else
    {
        pWall->cstat &= ~1;
        pWallNext->cstat &= ~1;
    }
    if (pFrame->at1_7)
    {
        pWall->cstat |= 64;
        pWallNext->cstat |= 64;
    }
    else
    {
        pWall->cstat &= ~64;
        pWallNext->cstat &= ~64;
    }
}

void UpdateFloor(int nXSector, SEQFRAME *pFrame)
{
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    int nSector = xsector[nXSector].reference;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    dassert(pSector->extra == nXSector);
    pSector->floorpicnum = seqGetTile(pFrame);
    pSector->floorshade = pFrame->at4_0;
    if (pFrame->at5_0)
        pSector->floorpal = pFrame->at5_0;
}

void UpdateCeiling(int nXSector, SEQFRAME *pFrame)
{
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    int nSector = xsector[nXSector].reference;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    dassert(pSector->extra == nXSector);
    pSector->ceilingpicnum = seqGetTile(pFrame);
    pSector->ceilingshade = pFrame->at4_0;
    if (pFrame->at5_0)
        pSector->ceilingpal = pFrame->at5_0;
}

void SEQINST::Update(ACTIVE *pActive)
{
    dassert(frameIndex < pSequence->nFrames);
    switch (pActive->type)
    {
    case 0:
        UpdateWall(pActive->xindex, &pSequence->frames[frameIndex]);
        break;
    case 1:
        UpdateCeiling(pActive->xindex , &pSequence->frames[frameIndex]);
        break;
    case 2:
        UpdateFloor(pActive->xindex, &pSequence->frames[frameIndex]);
        break;
    case 3: 
    {

        UpdateSprite(pActive->xindex, &pSequence->frames[frameIndex]);
        if (pSequence->frames[frameIndex].at6_1) {
            
            int sound = pSequence->ata;
            
            // by NoOne: add random sound range feature
            if (!VanillaMode() && pSequence->frames[frameIndex].soundRange > 0)
                sound += Random(((pSequence->frames[frameIndex].soundRange == 1) ? 2 : pSequence->frames[frameIndex].soundRange));
            
            sfxPlay3DSound(&sprite[xsprite[pActive->xindex].reference], sound, -1, 0);
        }

        
        // by NoOne: add surfaceSound trigger feature
        spritetype* pSprite = &sprite[xsprite[pActive->xindex].reference];
        if (!VanillaMode() && pSequence->frames[frameIndex].surfaceSound && zvel[pSprite->index] == 0 && xvel[pSprite->index] != 0) {
            
            if (gUpperLink[pSprite->sectnum] >= 0) break; // don't play surface sound for stacked sectors
            int surf = tileGetSurfType(pSprite->sectnum + 0x4000); if (!surf) break;
            static int surfSfxMove[15][4] = {
                /* {snd1, snd2, gameVolume, myVolume} */
                {800,801,80,25},
                {802,803,80,25},
                {804,805,80,25},
                {806,807,80,25},
                {808,809,80,25},
                {810,811,80,25},
                {812,813,80,25},
                {814,815,80,25},
                {816,817,80,25},
                {818,819,80,25},
                {820,821,80,25},
                {822,823,80,25},
                {824,825,80,25},
                {826,827,80,25},
                {828,829,80,25},
            };

            int sndId = surfSfxMove[surf][Random(2)];
            auto snd = soundEngine->FindSoundByResID(sndId);
            if (snd > 0)
            {
                auto udata = soundEngine->GetUserData(snd);
                int relVol = udata ? udata[2] : 255;
                sfxPlay3DSoundCP(pSprite, sndId, -1, 0, 0, (surfSfxMove[surf][2] != relVol) ? relVol : surfSfxMove[surf][3]);
            }
        }
        break;
    }
    case 4:
        UpdateMasked(pActive->xindex, &pSequence->frames[frameIndex]);
        break;
    }
    if (pSequence->frames[frameIndex].at5_5 && atc != -1)
        clientCallback[atc](pActive->type, pActive->xindex);
}

SEQINST * GetInstance(int a1, int a2)
{
    switch (a1)
    {
    case 0:
        if (a2 > 0 && a2 < kMaxXWalls) return &siWall[a2];
        break;
    case 1:
        if (a2 > 0 && a2 < kMaxXSectors) return &siCeiling[a2];
        break;
    case 2:
        if (a2 > 0 && a2 < kMaxXSectors) return &siFloor[a2];
        break;
    case 3:
        if (a2 > 0 && a2 < kMaxXSprites) return &siSprite[a2];
        break;
    case 4:
        if (a2 > 0 && a2 < kMaxWalls) return &siMasked[a2];
        break;
    }
    return NULL;
}

void UnlockInstance(SEQINST *pInst)
{
    dassert(pInst != NULL);
    dassert(pInst->pSequence != NULL);
    pInst->pSequence = NULL;
    pInst->at13 = 0;
}

void seqSpawn(int a1, int a2, int a3, int a4)
{
    SEQINST *pInst = GetInstance(a2, a3);
    if (!pInst) return;
    
    auto pSeq = getSequence(a1);
    if (!pSeq)
        ThrowError("Missing sequence #%d", a1);

    int i = activeCount;
    if (pInst->at13)
    {
        if (pSeq == pInst->pSequence)
            return;
        UnlockInstance(pInst);
        for (i = 0; i < activeCount; i++)
        {
            if (activeList[i].type == a2 && activeList[i].xindex == a3)
                break;
        }
        dassert(i < activeCount);
    }
    if (memcmp(pSeq->signature, "SEQ\x1a", 4) != 0)
        ThrowError("Invalid sequence %d", a1);
    if ((pSeq->version & 0xff00) != 0x300)
        ThrowError("Sequence %d is obsolete version", a1);
    if ((pSeq->version & 0xff) == 0x00)
    {
        for (int i = 0; i < pSeq->nFrames; i++)
            pSeq->frames[i].tile2 = 0;
    }
    pInst->at13 = 1;
    pInst->pSequence = pSeq;
    pInst->at8 = a1;
    pInst->atc = a4;
    pInst->at10 = pSeq->at8;
    pInst->frameIndex = 0;
    if (i == activeCount)
    {
        dassert(activeCount < kMaxSequences);
        activeList[activeCount].type = a2;
        activeList[activeCount].xindex = a3;
        activeCount++;
    }
    pInst->Update(&activeList[i]);
}

void seqKill(int a1, int a2)
{
    SEQINST *pInst = GetInstance(a1, a2);
    if (!pInst || !pInst->at13)
        return;
    int i;
    for (i = 0; i < activeCount; i++)
    {
        if (activeList[i].type == a1 && activeList[i].xindex == a2)
            break;
    }
    dassert(i < activeCount);
    activeCount--;
    activeList[i] = activeList[activeCount];
    pInst->at13 = 0;
    UnlockInstance(pInst);
}

void seqKillAll(void)
{
    for (int i = 0; i < kMaxXWalls; i++)
    {
        if (siWall[i].at13)
            UnlockInstance(&siWall[i]);
        if (siMasked[i].at13)
            UnlockInstance(&siMasked[i]);
    }
    for (int i = 0; i < kMaxXSectors; i++)
    {
        if (siCeiling[i].at13)
            UnlockInstance(&siCeiling[i]);
        if (siFloor[i].at13)
            UnlockInstance(&siFloor[i]);
    }
    for (int i = 0; i < kMaxXSprites; i++)
    {
        if (siSprite[i].at13)
            UnlockInstance(&siSprite[i]);
    }
    activeCount = 0;
}

int seqGetStatus(int a1, int a2)
{
    SEQINST *pInst = GetInstance(a1, a2);
    if (pInst && pInst->at13)
        return pInst->frameIndex;
    return -1;
}

int seqGetID(int a1, int a2)
{
    SEQINST *pInst = GetInstance(a1, a2);
    if (pInst)
        return pInst->at8;
    return -1;
}

void seqProcess(int a1)
{
    for (int i = 0; i < activeCount; i++)
    {
        SEQINST *pInst = GetInstance(activeList[i].type, activeList[i].xindex);
        Seq *pSeq = pInst->pSequence;
        dassert(pInst->frameIndex < pSeq->nFrames);
        pInst->at10 -= a1;
        while (pInst->at10 < 0)
        {
            pInst->at10 += pSeq->at8;
            pInst->frameIndex++;
            if (pInst->frameIndex == pSeq->nFrames)
            {
                if (pSeq->atc & 1)
                    pInst->frameIndex = 0;
                else
                {
                    UnlockInstance(pInst);
                    if (pSeq->atc & 2)
                    {
                        switch (activeList[i].type)
                        {
                        case 3:
                        {
                            int nXSprite = activeList[i].xindex;
                            int nSprite = xsprite[nXSprite].reference;
                            dassert(nSprite >= 0 && nSprite < kMaxSprites);
                            evKill(nSprite, 3);
                            if ((sprite[nSprite].flags & kHitagRespawn) && sprite[nSprite].inittype >= kDudeBase && sprite[nSprite].inittype < kDudeMax)
                                evPost(nSprite, 3, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
                            else
                                DeleteSprite(nSprite);
                            break;
                        }
                        case 4:
                        {
                            int nXWall = activeList[i].xindex;
                            int nWall = xwall[nXWall].reference;
                            dassert(nWall >= 0 && nWall < kMaxWalls);
                            wall[nWall].cstat &= ~(8 + 16 + 32);
                            if (wall[nWall].nextwall != -1)
                                wall[wall[nWall].nextwall].cstat &= ~(8 + 16 + 32);
                            break;
                        }
                        }
                    }
                    activeList[i--] = activeList[--activeCount];
                    break;
                }
            }
            pInst->Update(&activeList[i]);
        }
    }
}

class SeqLoadSave : public LoadSave {
    virtual void Load(void);
    virtual void Save(void);
};

void SeqLoadSave::Load(void)
{
    Read(&siWall, sizeof(siWall));
    Read(&siMasked, sizeof(siMasked));
    Read(&siCeiling, sizeof(siCeiling));
    Read(&siFloor, sizeof(siFloor));
    Read(&siSprite, sizeof(siSprite));
    Read(&activeList, sizeof(activeList));
    Read(&activeCount, sizeof(activeCount));
    for (int i = 0; i < kMaxXWalls; i++)
    {
        siWall[i].pSequence = NULL;
        siMasked[i].pSequence = NULL;
    }
    for (int i = 0; i < kMaxXSectors; i++)
    {
        siCeiling[i].pSequence = NULL;
        siFloor[i].pSequence = NULL;
    }
    for (int i = 0; i < kMaxXSprites; i++)
    {
        siSprite[i].pSequence = NULL;
    }
    for (int i = 0; i < activeCount; i++)
    {
        SEQINST *pInst = GetInstance(activeList[i].type, activeList[i].xindex);
        if (pInst->at13)
        {
            int nSeq = pInst->at8;
            auto pSeq = getSequence(nSeq);
            if (!pSeq) {
                ThrowError("Missing sequence #%d", nSeq);
                continue;
            }
            if (memcmp(pSeq->signature, "SEQ\x1a", 4) != 0)
                ThrowError("Invalid sequence %d", nSeq);
            if ((pSeq->version & 0xff00) != 0x300)
                ThrowError("Sequence %d is obsolete version", nSeq);
            pInst->pSequence = pSeq;
        }
    }
}

void SeqLoadSave::Save(void)
{
    Write(&siWall, sizeof(siWall));
    Write(&siMasked, sizeof(siMasked));
    Write(&siCeiling, sizeof(siCeiling));
    Write(&siFloor, sizeof(siFloor));
    Write(&siSprite, sizeof(siSprite));
    Write(&activeList, sizeof(activeList));
    Write(&activeCount, sizeof(activeCount));
}

static SeqLoadSave *myLoadSave;

void SeqLoadSaveConstruct(void)
{
    myLoadSave = new SeqLoadSave();
}


static void ByteSwapSEQ(Seq* pSeq)
{
#if B_BIG_ENDIAN == 1
    pSeq->version = B_LITTLE16(pSeq->version);
    pSeq->nFrames = B_LITTLE16(pSeq->nFrames);
    pSeq->at8 = B_LITTLE16(pSeq->at8);
    pSeq->ata = B_LITTLE16(pSeq->ata);
    pSeq->atc = B_LITTLE32(pSeq->atc);
    for (int i = 0; i < pSeq->nFrames; i++)
    {
        SEQFRAME* pFrame = &pSeq->frames[i];
        BitReader bitReader((char*)pFrame, sizeof(SEQFRAME));
        SEQFRAME swapFrame;
        swapFrame.tile = bitReader.readUnsigned(12);
        swapFrame.at1_4 = bitReader.readBit();
        swapFrame.at1_5 = bitReader.readBit();
        swapFrame.at1_6 = bitReader.readBit();
        swapFrame.at1_7 = bitReader.readBit();
        swapFrame.at2_0 = bitReader.readUnsigned(8);
        swapFrame.at3_0 = bitReader.readUnsigned(8);
        swapFrame.at4_0 = bitReader.readSigned(8);
        swapFrame.at5_0 = bitReader.readUnsigned(5);
        swapFrame.at5_5 = bitReader.readBit();
        swapFrame.at5_6 = bitReader.readBit();
        swapFrame.at5_7 = bitReader.readBit();
        swapFrame.at6_0 = bitReader.readBit();
        swapFrame.at6_1 = bitReader.readBit();
        swapFrame.at6_2 = bitReader.readBit();
        swapFrame.at6_3 = bitReader.readBit();
        swapFrame.at6_4 = bitReader.readBit();
        swapFrame.tile2 = bitReader.readUnsigned(4);
        swapFrame.soundRange = bitReader.readUnsigned(4);
        swapFrame.surfaceSound = bitReader.readBit();
        swapFrame.reserved = bitReader.readUnsigned(2);
        *pFrame = swapFrame;
    }
#endif
}


// This is to eliminate a huge design issue in NBlood that was apparently copied verbatim from the DOS-Version.
// Sequences were cached in the resource and directly returned from there in writable form, with byte swapping directly performed in the cache on Big Endian systems.
// To avoid such unsafe operations this caches the read data separately in static memory that's guaranteed not to be touched by the file system.
FMemArena seqcache;
static TMap<int, Seq*> sequences;
Seq* getSequence(int res_id)
{
    auto p = sequences.CheckKey(res_id);
    if (p != nullptr) return *p;

    int index = fileSystem.FindResource(res_id, "SEQ");
    if (index < 0)
    {
        return nullptr;
    }
    auto fr = fileSystem.OpenFileReader(index);
    auto seqdata = (Seq*)seqcache.Alloc(fr.GetLength());
    fr.Read(seqdata, fr.GetLength());
    sequences.Insert(res_id, seqdata);
    ByteSwapSEQ(seqdata);
    return seqdata;
}

END_BLD_NS
