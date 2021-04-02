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
#include "aistuff.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

struct Lava
{
    short nSprite;
    short nRun;
    short nAction;
    short nTarget;
    short nHealth;
    short nFrame;
    short nIndex;
};

TArray<Lava> LavaList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Lava& w, Lava* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("run", w.nRun)
            ("channel", w.nIndex)
            .EndObject();
    }
    return arc;
}

void SerializeLavadude(FSerializer& arc)
{
    arc("lavadude", LavaList);
}


static actionSeq LavadudeSeq[] = {
    {0, 1},
    {0, 1},
    {1, 0},
    {10, 0},
    {19, 0},
    {28, 1},
    {29, 1},
    {33, 0},
    {42, 1}
};

void InitLava()
{
    LavaList.Clear();
}

int BuildLavaLimb(int nSprite, int edx, int ebx)
{
    short nSector = sprite[nSprite].sectnum;

    int nLimbSprite = insertsprite(nSector, 118);
    assert(nLimbSprite >= 0 && nLimbSprite < kMaxSprites);

    sprite[nLimbSprite].x = sprite[nSprite].x;
    sprite[nLimbSprite].y = sprite[nSprite].y;
    sprite[nLimbSprite].z = sprite[nSprite].z - RandomLong() % ebx;
    sprite[nLimbSprite].cstat = 0;
    sprite[nLimbSprite].shade = -127;
    sprite[nLimbSprite].pal = 1;
    sprite[nLimbSprite].xvel = (RandomSize(5) - 16) << 8;
    sprite[nLimbSprite].yvel = (RandomSize(5) - 16) << 8;
    sprite[nLimbSprite].zvel = 2560 - (RandomSize(5) << 8);
    sprite[nLimbSprite].xoffset = 0;
    sprite[nLimbSprite].yoffset = 0;
    sprite[nLimbSprite].xrepeat = 90;
    sprite[nLimbSprite].yrepeat = 90;
    sprite[nLimbSprite].picnum = (edx & 3) % 3;
    sprite[nLimbSprite].hitag = 0;
    sprite[nLimbSprite].lotag = runlist_HeadRun() + 1;
    sprite[nLimbSprite].clipdist = 0;

//	GrabTimeSlot(3);

    sprite[nLimbSprite].extra = -1;
    sprite[nLimbSprite].owner = runlist_AddRunRec(sprite[nLimbSprite].lotag - 1, nLimbSprite | 0x160000);
    sprite[nLimbSprite].hitag = runlist_AddRunRec(NewRun, nLimbSprite | 0x160000);

    return nLimbSprite;
}

void FuncLavaLimb(int a, int, int nRun)
{
    short nSprite = RunData[nRun].nVal;
    assert(nSprite >= 0 && nSprite < kMaxSprites);

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
            sprite[nSprite].shade += 3;

            int nRet = movesprite(nSprite, sprite[nSprite].xvel << 12, sprite[nSprite].yvel << 12, sprite[nSprite].zvel, 2560, -2560, CLIPMASK1);

            if (nRet || sprite[nSprite].shade > 100)
            {
                sprite[nSprite].xvel = 0;
                sprite[nSprite].yvel = 0;
                sprite[nSprite].zvel = 0;

                runlist_DoSubRunRec(sprite[nSprite].owner);
                runlist_FreeRun(sprite[nSprite].lotag - 1);
                runlist_SubRunRec(sprite[nSprite].hitag);

                mydeletesprite(nSprite);
            }
            break;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, (SeqOffsets[kSeqLavag] + 30) + sprite[nSprite].picnum, 0, 1);
            break;
        }

        default:
            return;
    }
}

int BuildLava(short nSprite, int x, int y, int, short nSector, short nAngle, int nChannel)
{
    auto nLava = LavaList.Reserve(1);

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 118);
    }
    else
    {
        nSector = sprite[nSprite].sectnum;
        nAngle = sprite[nSprite].ang;
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;

        changespritestat(nSprite, 118);
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = sector[nSector].floorz;
    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].xrepeat = 200;
    sprite[nSprite].yrepeat = 200;
    sprite[nSprite].shade = -12;
    sprite[nSprite].pal = 0;
    sprite[nSprite].clipdist = 127;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = seq_GetSeqPicnum(kSeqLavag, LavadudeSeq[3].a, 0);
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;

//	GrabTimeSlot(3);

    sprite[nSprite].extra = -1;

    LavaList[nLava].nAction = 0;
    LavaList[nLava].nHealth = 4000;
    LavaList[nLava].nSprite = nSprite;
    LavaList[nLava].nTarget = -1;
    LavaList[nLava].nIndex = nChannel;
    LavaList[nLava].nFrame = 0;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nLava | 0x150000);
    LavaList[nLava].nRun = runlist_AddRunRec(NewRun, nLava | 0x150000);

    nCreaturesTotal++;

    return nLava | 0x150000;
}

void FuncLava(int a, int nDamage, int nRun)
{
    short nLava = RunData[nRun].nVal;
    assert(nLava >= 0 && nLava < LavaList.Size());

    short nAction = LavaList[nLava].nAction;
    short nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];
    short nSprite = LavaList[nLava].nSprite;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Lava\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, nSeq, LavaList[nLava].nFrame, LavadudeSeq[nAction].b);
            mytsprite[a & 0xFFFF].owner = -1;
            return;
        }

        case 0xA0000:
        {
            return;
        }

        case 0x80000:
        {
            if (!nDamage) {
                return;
            }

            LavaList[nLava].nHealth -= dmgAdjust(nDamage, 3);

            if (LavaList[nLava].nHealth <= 0)
            {
                LavaList[nLava].nHealth = 0;
                LavaList[nLava].nAction = 5;
                LavaList[nLava].nFrame  = 0;

                nCreaturesKilled++;

                sprite[nSprite].cstat &= 0xFEFE;
            }
            else
            {
                short nTarget = a & 0xFFFF;

                if (nTarget >= 0)
                {
                    if (sprite[nTarget].statnum < 199)
                    {
                        LavaList[nLava].nTarget = nTarget;
                    }
                }

                if (nAction == 3)
                {
                    if (!RandomSize(2))
                    {
                        LavaList[nLava].nAction = 4;
                        LavaList[nLava].nFrame  = 0;
                        sprite[nSprite].cstat = 0;
                    }
                }

                BuildLavaLimb(nSprite, totalmoves, 64000);
            }

            return;
        }

        case 0x20000:
        {
            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, LavaList[nLava].nFrame);
            int var_38 = LavaList[nLava].nFrame;

            short nFlag = FrameFlag[SeqBase[nSeq] + var_38];

            int var_1C;

            if (nAction)
            {
                seq_MoveSequence(nSprite, nSeq, var_38);

                LavaList[nLava].nFrame++;
                if (LavaList[nLava].nFrame >= SeqSize[nSeq])
                {
                    var_1C = 1;
                    LavaList[nLava].nFrame = 0;
                }
                else
                {
                    var_1C = 0;
                }
            }

            short nTarget = LavaList[nLava].nTarget;

            if (nTarget >= 0 && nAction < 4)
            {
                if (!(sprite[nTarget].cstat & 0x101) || sprite[nTarget].sectnum >= 1024)
                {
                    nTarget = -1;
                    LavaList[nLava].nTarget = -1;
                }
            }

            switch (nAction)
            {
                case 0:
                {
                    if ((nLava & 0x1F) == (totalmoves & 0x1F))
                    {
                        if (nTarget < 0)
                        {
                            nTarget = FindPlayer(nSprite, 76800);
                        }

                        PlotCourseToSprite(nSprite, nTarget);

                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang);

                        if (nTarget >= 0 && !RandomSize(1))
                        {
                            LavaList[nLava].nTarget = nTarget;
                            LavaList[nLava].nAction = 2;
                            sprite[nSprite].cstat = 0x101;
                            LavaList[nLava].nFrame = 0;
                            break;
                        }
                    }

                    int x = sprite[nSprite].x;
                    int y = sprite[nSprite].y;
                    int z = sprite[nSprite].z;
                    short nSector = sprite[nSprite].sectnum;

                    int nVal = movesprite(nSprite, sprite[nSprite].xvel << 8, sprite[nSprite].yvel << 8, 0, 0, 0, CLIPMASK0);

                    if (nSector != sprite[nSprite].sectnum)
                    {
                        changespritesect(nSprite, nSector);
                        sprite[nSprite].x = x;
                        sprite[nSprite].y = y;
                        sprite[nSprite].z = z;

                        sprite[nSprite].ang = (sprite[nSprite].ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang);
                        break;
                    }

                    if (!nVal) {
                        break;
                    }

                    if ((nVal & 0xC000) == 0x8000)
                    {
                        sprite[nSprite].ang = (sprite[nSprite].ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang);
                        break;
                    }
                    else if ((nVal & 0xC000) == 0xC000)
                    {
                        if ((nVal & 0x3FFF) == nTarget)
                        {
                            int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                            if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                            {
                                LavaList[nLava].nAction = 2;
                                LavaList[nLava].nFrame = 0;
                                sprite[nSprite].cstat = 0x101;
                                break;
                            }
                        }
                    }

                    break;
                }

                case 1:
                case 6:
                {
                    break;
                }

                case 2:
                {
                    if (var_1C)
                    {
                        LavaList[nLava].nAction = 3;
                        LavaList[nLava].nFrame = 0;

                        PlotCourseToSprite(nSprite, nTarget);

                        sprite[nSprite].cstat |= 0x101;
                    }

                    break;
                }

                case 3:
                {
                    if ((nFlag & 0x80) && nTarget > -1)
                    {
                        int nHeight = GetSpriteHeight(nSprite);
                        GetUpAngle(nSprite, -64000, nTarget, (-(nHeight >> 1)));

                        BuildBullet(nSprite, 10, bcos(sprite[nSprite].ang, 8), bsin(sprite[nSprite].ang, 8), -1, sprite[nSprite].ang, nTarget + 10000, 1);
                    }
                    else if (var_1C)
                    {
                        PlotCourseToSprite(nSprite, nTarget);
                        LavaList[nLava].nAction = 7;
                        LavaList[nLava].nFrame = 0;
                    }

                    break;
                }

                case 4:
                {
                    if (var_1C)
                    {
                        LavaList[nLava].nAction = 7;
                        sprite[nSprite].cstat &= 0xFEFE;
                    }

                    break;
                }

                case 5:
                {
                    if (nFlag & 0x40)
                    {
                        int nLimbSprite = BuildLavaLimb(nSprite, LavaList[nLava].nFrame, 64000);
                        D3PlayFX(StaticSound[kSound26], nLimbSprite);
                    }

                    if (LavaList[nLava].nFrame)
                    {
                        if (nFlag & 0x80)
                        {
                            int ecx = 0;
                            do
                            {
                                BuildLavaLimb(nSprite, ecx, 64000);
                                ecx++;
                            }
                            while (ecx < 20);
                            runlist_ChangeChannel(LavaList[nLava].nIndex, 1);
                        }
                    }
                    else
                    {
                        int ecx = 0;

                        do
                        {
                            BuildLavaLimb(nSprite, ecx, 256);
                            ecx++;
                        }
                        while (ecx < 30);

                        runlist_DoSubRunRec(sprite[nSprite].owner);
                        runlist_FreeRun(sprite[nSprite].lotag - 1);
                        runlist_SubRunRec(LavaList[nLava].nRun);
                        mydeletesprite(nSprite);
                    }

                    break;
                }

                case 7:
                {
                    if (var_1C)
                    {
                        LavaList[nLava].nAction = 8;
                        LavaList[nLava].nFrame = 0;
                    }
                    break;
                }

                case 8:
                {
                    if (var_1C)
                    {
                        LavaList[nLava].nAction = 0;
                        LavaList[nLava].nFrame = 0;
                        sprite[nSprite].cstat = 0x8000;
                    }
                    break;
                }
            }

            // loc_31521:
            sprite[nSprite].pal = 1;
        }
    }
}
END_PS_NS
