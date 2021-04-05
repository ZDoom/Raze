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
#include "aistuff.h"
#include "engine.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq FishSeq[] = {
    {8, 0},
    {8, 0},
    {0, 0},
    {24, 0},
    {8, 0},
    {32, 1},
    {33, 1},
    {34, 1},
    {35, 1},
    {39, 1}
};


struct Fish
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nCount;
    short nRun;
};

struct Chunk
{
    short nSprite;
    short nIndex;
    short nSeqIndex;
};

TArray<Fish> FishList;
TArray<Chunk> FishChunk;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Fish& w, Fish* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("run", w.nRun)
            ("count", w.nCount)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Chunk& w, Chunk* def)
{
    if (arc.BeginObject(keyname))
    {
        arc ("sprite", w.nSprite)
            ("index", w.nIndex)
            ("seqindex", w.nSeqIndex)
            .EndObject();
    }
    return arc;
}

void SerializeFish(FSerializer& arc)
{
    arc("fish", FishList)
        ("fishchunk", FishChunk);
}

void InitFishes()
{
    FishList.Clear();
    FishChunk.Clear();
}

int BuildFishLimb(short nFish, short edx)
{
    short nSprite = FishList[nFish].nSprite;

    int nFree = FishChunk.Reserve(1);

    int nSprite2 = insertsprite(sprite[nSprite].sectnum, 99);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);

    FishChunk[nFree].nSprite = nSprite2;
    FishChunk[nFree].nSeqIndex = edx + 40;
    FishChunk[nFree].nIndex = RandomSize(3) % SeqSize[SeqOffsets[kSeqFish] + edx + 40];

    sprite[nSprite2].x = sprite[nSprite].x;
    sprite[nSprite2].y = sprite[nSprite].y;
    sprite[nSprite2].z = sprite[nSprite].z;
    sprite[nSprite2].cstat = 0;
    sprite[nSprite2].shade = -12;
    sprite[nSprite2].pal = 0;
    sprite[nSprite2].xvel = (RandomSize(5) - 16) << 8;
    sprite[nSprite2].yvel = (RandomSize(5) - 16) << 8;
    sprite[nSprite2].xrepeat = 64;
    sprite[nSprite2].yrepeat = 64;
    sprite[nSprite2].xoffset = 0;
    sprite[nSprite2].yoffset = 0;
    sprite[nSprite2].zvel = (-(RandomByte() + 512)) * 2;

    seq_GetSeqPicnum(kSeqFish, FishChunk[nFree].nSeqIndex, 0);

    sprite[nSprite2].picnum = edx;
    sprite[nSprite2].lotag = runlist_HeadRun() + 1;
    sprite[nSprite2].clipdist = 0;

//	GrabTimeSlot(3);

    sprite[nSprite2].extra = -1;
    sprite[nSprite2].owner = runlist_AddRunRec(sprite[nSprite2].lotag - 1, nFree | 0x200000);
    sprite[nSprite2].hitag = runlist_AddRunRec(NewRun, nFree | 0x200000);

    return nFree | 0x200000;
}

void BuildBlood(int x, int y, int z, short nSector)
{
    BuildAnim(-1, kSeqFish, 36, x, y, z, nSector, 75, 128);
}

void FuncFishLimb(int a, int, int nRun)
{
    short nFish = RunData[nRun].nVal;
    short nSprite = FishChunk[nFish].nSprite;
    assert(nSprite >= 0 && nSprite < kMaxSprites);

    int nSeq = SeqOffsets[kSeqFish] + FishChunk[nFish].nSeqIndex;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, FishChunk[nFish].nIndex);

            Gravity(nSprite);

            FishChunk[nFish].nIndex++;

            if (FishChunk[nFish].nIndex >= SeqSize[nSeq])
            {
                FishChunk[nFish].nIndex = 0;
                if (RandomBit()) {
                    BuildBlood(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum);
                }
            }

            int FloorZ = sector[sprite[nSprite].sectnum].floorz;

            if (FloorZ <= sprite[nSprite].z)
            {
                sprite[nSprite].z += 256;

                if ((sprite[nSprite].z - FloorZ) > 25600)
                {
                    sprite[nSprite].zvel = 0;
                    runlist_DoSubRunRec(sprite[nSprite].owner);
                    runlist_FreeRun(sprite[nSprite].lotag - 1);
                    runlist_SubRunRec(sprite[nSprite].hitag);
                    mydeletesprite(nSprite);
                }
                else if ((sprite[nSprite].z - FloorZ) > 0)
                {
                    sprite[nSprite].zvel = 1024;
                }

                return;
            }
            else
            {
                if (movesprite(nSprite, sprite[nSprite].xvel << 8, sprite[nSprite].yvel << 8, sprite[nSprite].zvel, 2560, -2560, CLIPMASK1))
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                }
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, nSeq, FishChunk[nFish].nIndex, 1);
            return;
        }
    }
}

int BuildFish(int nSprite, int x, int y, int z, int nSector, int nAngle)
{
    int nFish = FishList.Reserve(1);

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 103);
    }
    else
    {
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sprite[nSprite].z;
        nAngle = sprite[nSprite].ang;
        changespritestat(nSprite, 103);
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].shade = -12;
    sprite[nSprite].clipdist = 80;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = seq_GetSeqPicnum(kSeqFish, FishSeq[0].a, 0);
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

    FishList[nFish].nAction = 0;
    FishList[nFish].nHealth = 200;
    FishList[nFish].nSprite = nSprite;
    FishList[nFish].nTarget = -1;
    FishList[nFish].nCount = 60;
    FishList[nFish].nFrame = 0;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nFish | 0x120000);
    FishList[nFish].nRun = runlist_AddRunRec(NewRun, nFish | 0x120000);

    nCreaturesTotal++;

    return nFish | 0x120000;
}

void IdleFish(short nFish, short edx)
{
    short nSprite = FishList[nFish].nSprite;

    sprite[nSprite].ang += (256 - RandomSize(9)) + 1024;
    sprite[nSprite].ang &= kAngleMask;

    sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -8);
    sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -8);

    FishList[nFish].nAction = 0;
    FishList[nFish].nFrame = 0;

    sprite[nSprite].zvel = RandomSize(9);

    if (!edx)
    {
        if (RandomBit()) {
            sprite[nSprite].zvel = -sprite[nSprite].zvel;
        }
    }
    else if (edx < 0)
    {
        sprite[nSprite].zvel = -sprite[nSprite].zvel;
    }
}

void DestroyFish(short nFish)
{
    short nSprite = FishList[nFish].nSprite;

    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_FreeRun(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(FishList[nFish].nRun);
    mydeletesprite(nSprite);
}

void FuncFish(int a, int nDamage, int nRun)
{
    short nFish = RunData[nRun].nVal;
    assert(nFish >= 0 && nFish < (int)FishList.Size());

    short nSprite = FishList[nFish].nSprite;
    short nAction = FishList[nFish].nAction;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Fish\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqFish] + FishSeq[nAction].a, FishList[nFish].nFrame, FishSeq[nAction].b);
            mytsprite[a & 0xFFFF].owner = -1;
            return;
        }

        case 0xA0000:
        {
            if (FishList[nFish].nHealth <= 0) {
                return;
            }
            else
            {
                nDamage = runlist_CheckRadialDamage(nSprite);
                if (!nDamage) {
                    return;
                }

                FishList[nFish].nCount = 10;
            }
            // fall through
            fallthrough__;
        }
        case 0x80000:
        {
            if (!nDamage) {
                return;
            }

            FishList[nFish].nHealth -= dmgAdjust(nDamage);
            if (FishList[nFish].nHealth <= 0)
            {
                FishList[nFish].nHealth = 0;
                nCreaturesKilled++;

                sprite[nSprite].cstat &= 0xFEFE;

                if (nMessage == 0x80000)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        BuildFishLimb(nFish, i);
                    }

                    PlayFXAtXYZ(StaticSound[kSound40], sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum);
                    DestroyFish(nFish);
                }
                else
                {
                    FishList[nFish].nAction = 9;
                    FishList[nFish].nFrame = 0;
                }

                return;
            }
            else
            {
                short nTarget = a & 0xFFFF;
                if (nTarget >= 0 && sprite[nTarget].statnum < 199)
                {
                    FishList[nFish].nTarget = nTarget;
                }

                FishList[nFish].nAction = 4;
                FishList[nFish].nFrame = 0;
                FishList[nFish].nCount += 10;
            }

            return;
        }

        case 0x20000:
        {
            if (!(SectFlag[sprite[nSprite].sectnum] & kSectUnderwater))
            {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqFish] + FishSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, FishList[nFish].nFrame);

            seq_MoveSequence(nSprite, nSeq, FishList[nFish].nFrame);

            FishList[nFish].nFrame++;
            if (FishList[nFish].nFrame >= SeqSize[nSeq]) {
                FishList[nFish].nFrame = 0;
            }

            short nTarget = FishList[nFish].nTarget;

            switch (nAction)
            {
                default:
                    return;

                case 0:
                {
                    FishList[nFish].nCount--;
                    if (FishList[nFish].nCount <= 0)
                    {
                        nTarget = FindPlayer(nSprite, 60);
                        if (nTarget >= 0)
                        {
                            FishList[nFish].nTarget = nTarget;
                            FishList[nFish].nAction = 2;
                            FishList[nFish].nFrame = 0;

                            int nAngle = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].z - sprite[nSprite].z);
                            sprite[nSprite].zvel = bsin(nAngle, -5);

                            FishList[nFish].nCount = RandomSize(6) + 90;
                        }
                        else
                        {
                            IdleFish(nFish, 0);
                        }
                    }

                    break;
                }

                case 1:
                    return;

                case 2:
                case 3:
                {
                    FishList[nFish].nCount--;
                    if (FishList[nFish].nCount <= 0)
                    {
                        IdleFish(nFish, 0);
                        return;
                    }
                    else
                    {
                        PlotCourseToSprite(nSprite, nTarget);
                        int nHeight = GetSpriteHeight(nSprite) >> 1;

                        int z = abs(sprite[nTarget].z - sprite[nSprite].z);

                        if (z <= nHeight)
                        {
                            sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -5) - bcos(sprite[nSprite].ang, -7);
                            sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -5) - bsin(sprite[nSprite].ang, -7);
                        }
                        else
                        {
                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
                        }

                        sprite[nSprite].zvel = (sprite[nTarget].z - sprite[nSprite].z) >> 3;
                    }
                    break;
                }

                case 4:
                {
                    if (FishList[nFish].nFrame == 0)
                    {
                        IdleFish(nFish, 0);
                    }
                    return;
                }

                case 8:
                {
                    return;
                }

                case 9:
                {
                    if (FishList[nFish].nFrame == 0)
                    {
                        DestroyFish(nFish);
                    }
                    return;
                }
            }

            int x = sprite[nSprite].x;
            int y = sprite[nSprite].y;
            int z = sprite[nSprite].z;
            short nSector = sprite[nSprite].sectnum;

            // loc_2EF54
            int nMov = movesprite(nSprite, sprite[nSprite].xvel << 13, sprite[nSprite].yvel << 13, sprite[nSprite].zvel << 2, 0, 0, CLIPMASK0);

            if (!(SectFlag[sprite[nSprite].sectnum] & kSectUnderwater))
            {
                mychangespritesect(nSprite, nSector);
                sprite[nSprite].x = x;
                sprite[nSprite].y = y;
                sprite[nSprite].z = z;

                IdleFish(nFish, 0);
                return;
            }
            else
            {
                if (nAction >= 5) {
                    return;
                }

                if (!nMov)
                {
                    if (nAction == 3)
                    {
                        FishList[nFish].nAction = 2;
                        FishList[nFish].nFrame = 0;
                    }
                    return;
                }

                if ((nMov & 0x30000) == 0)
                {
                    if ((nMov & 0xC000) == 0x8000)
                    {
                        IdleFish(nFish, 0);
                    }
                    else if ((nMov & 0xC000) == 0xC000)
                    {
                        if (sprite[nMov & 0x3FFF].statnum == 100)
                        {
                            FishList[nFish].nTarget = nMov & 0x3FFF;
                            sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                            if (nAction != 3)
                            {
                                FishList[nFish].nAction = 3;
                                FishList[nFish].nFrame = 0;
                            }

                            if (!FishList[nFish].nFrame)
                            {
                                runlist_DamageEnemy(nTarget, nSprite, 2);
                            }
                        }
                    }
                }
                else if (nMov & 0x20000)
                {
                    IdleFish(nFish, -1);
                }
                else
                {
                    IdleFish(nFish, 1);
                }
            }

            return;
        }
    }
}
END_PS_NS
