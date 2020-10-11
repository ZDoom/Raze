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
#include "exhumed.h"
#include "sequence.h"
#include <assert.h>

BEGIN_PS_NS

#define kMaxSets	10

short SetCount = 0;

static actionSeq SetSeq[] = {
    {0, 0},
    {77, 1},
    {78, 1},
    {0, 0},
    {9, 0},
    {63, 0},
    {45, 0},
    {18, 0},
    {27, 0},
    {36, 0},
    {72, 1},
    {74, 1}
};

struct Set
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short field_D;
    short field_E;
};

Set SetList[kMaxSets];
short SetChan[kMaxSets];

static SavegameHelper sghset("set",
    SV(SetCount),
    SA(SetList),
    SA(SetChan),
    nullptr);


void InitSets()
{
    SetCount = kMaxSets;
}

int BuildSet(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    SetCount--;

    short nSet = SetCount;
    if (nSet < 0) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 120);
    }
    else
    {
        changespritestat(nSprite, 120);
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sector[sprite[nSprite].sectnum].floorz;
        nAngle = sprite[nSprite].ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].shade = -12;
    sprite[nSprite].clipdist = 110;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].xrepeat = 87;
    sprite[nSprite].yrepeat = 96;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;

    //	GrabTimeSlot(3);

    SetList[nSet].nAction = 1;
    SetList[nSet].nHealth = 8000;
    SetList[nSet].nSprite = nSprite;
    SetList[nSet].nFrame  = 0;
    SetList[nSet].nTarget = -1;
    SetList[nSet].field_A = 90;
    SetList[nSet].field_C = 0;
    SetList[nSet].field_D = 0;

    SetChan[nSet] = nChannel;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSet | 0x190000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, nSet | 0x190000);

    nCreaturesTotal++;

    return nSet | 0x190000;
}

int BuildSoul(int nSet)
{
    int nSetSprite = SetList[nSet].nSprite;
    int nSprite = insertsprite(sprite[nSetSprite].sectnum, 0);

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].shade = -127;
    sprite[nSprite].xrepeat = 1;
    sprite[nSprite].yrepeat = 1;
    sprite[nSprite].pal = 0;
    sprite[nSprite].clipdist = 5;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = seq_GetSeqPicnum(kSeqSet, 75, 0);
    sprite[nSprite].ang = RandomSize(11);
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = (-256) - RandomSize(10);
    sprite[nSprite].x = sprite[nSetSprite].x;
    sprite[nSprite].y = sprite[nSetSprite].y;

    short nSector = sprite[nSprite].sectnum;
    sprite[nSprite].z = (RandomSize(8) << 8) + 8192 + sector[nSector].ceilingz - GetSpriteHeight(nSprite);

    sprite[nSprite].hitag = nSet;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = 0;

//	GrabTimeSlot(3);

    sprite[nSprite].owner = runlist_AddRunRec(NewRun, nSprite | 0x230000);

    return nSprite | 0x230000;
}

void FuncSoul(int a, int, int nRun)
{
    short nSprite = RunData[nRun].nVal;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
            seq_MoveSequence(nSprite, SeqOffsets[kSeqSet] + 75, 0);

            if (sprite[nSprite].xrepeat < 32)
            {
                sprite[nSprite].xrepeat++;
                sprite[nSprite].yrepeat++;
            }

            sprite[nSprite].extra += (nSprite & 0x0F) + 5;
            sprite[nSprite].extra &= kAngleMask;

            int nVel = (Cos(sprite[nSprite].extra) >> 7);

            if (movesprite(nSprite, Cos(sprite[nSprite].ang) * nVel, Sin(sprite[nSprite].ang) * nVel, sprite[nSprite].zvel, 5120, 0, CLIPMASK0) & 0x10000)
            {
                int nSet = sprite[nSprite].hitag;
                int nSetSprite = SetList[nSet].nSprite;

                sprite[nSprite].cstat = 0;
                sprite[nSprite].yrepeat = 1;
                sprite[nSprite].xrepeat = 1;
                sprite[nSprite].x = sprite[nSetSprite].x;
                sprite[nSprite].y = sprite[nSetSprite].y;
                sprite[nSprite].z = sprite[nSetSprite].z - (GetSpriteHeight(nSetSprite) >> 1);
                mychangespritesect(nSprite, sprite[nSetSprite].sectnum);
                return;
            }
        }

        case 0x80000:
        case 0xA0000:
        case 0x90000:
            return;

        default:
            Printf("unknown msg %d for Soul\n", nMessage);
    }
}

void FuncSet(int a, int nDamage, int nRun)
{
    short nSet = RunData[nRun].nVal;
    assert(nSet >= 0 && nSet < kMaxSets);

    short nSprite = SetList[nSet].nSprite;
    short nAction = SetList[nSet].nAction;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Set\n", nMessage);
            return;
        }

        case 0xA0000:
        {
            if (nAction == 5)
            {
                nDamage = runlist_CheckRadialDamage(nSprite);
                // fall through to case 0x80000
            }
            fallthrough__;
        }

        case 0x80000:
        {
            if (nDamage && SetList[nSet].nHealth > 0)
            {
                if (nAction != 1)
                {
                    SetList[nSet].nHealth -= nDamage;
                }

                if (SetList[nSet].nHealth <= 0)
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                    sprite[nSprite].cstat &= 0xFEFE;

                    SetList[nSet].nHealth = 0;

                    nCreaturesKilled++;

                    if (nAction < 10)
                    {
                        SetList[nSet].nFrame  = 0;
                        SetList[nSet].nAction = 10;
                    }
                }
                else if (nAction == 1)
                {
                    SetList[nSet].nAction = 2;
                    SetList[nSet].nFrame  = 0;
                }
            }
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a, SeqOffsets[kSeqSet] + SetSeq[nAction].a, SetList[nSet].nFrame, SetSeq[nAction].b);
            return;
        }

        case 0x20000:
        {
            Gravity(nSprite);

            short nSeq = SeqOffsets[kSeqSet] + SetSeq[SetList[nSet].nAction].a;
            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, SetList[nSet].nFrame);
            seq_MoveSequence(nSprite, nSeq, SetList[nSet].nFrame);

            if (nAction == 3)
            {
                if (SetList[nSet].field_D) {
                    SetList[nSet].nFrame++;
                }
            }

            SetList[nSet].nFrame++;
            if (SetList[nSet].nFrame >= SeqSize[nSeq])
            {
                SetList[nSet].nFrame = 0;
                bVal = true;
            }

            short nFlag = FrameFlag[SeqBase[nSeq] + SetList[nSet].nFrame];
            short nTarget = SetList[nSet].nTarget;

            if (nTarget > -1 && nAction < 10)
            {
                if (!(sprite[nTarget].cstat & 0x101))
                {
                    SetList[nSet].nTarget = -1;
                    SetList[nSet].nAction = 0;
                    SetList[nSet].nFrame  = 0;
                    nTarget = -1;
                }
            }

            int nMov = MoveCreature(nSprite);

            pushmove_old(&sprite[nSprite].x, &sprite[nSprite].y, &sprite[nSprite].z, &sprite[nSprite].sectnum, sprite[nSprite].clipdist << 2, 5120, -5120, CLIPMASK0);

            if (sprite[nSprite].zvel > 4000)
            {
                if (nMov & 0x20000)
                {
                    SetQuake(nSprite, 100);
                }
            }

            switch (nAction)
            {
                default:
                    return;

                case 0:
                {
                    if ((nSet & 0x1F) == (totalmoves & 0x1F))
                    {
                        if (nTarget < 0)
                        {
                            nTarget = FindPlayer(nSprite, 1000);
                        }

                        if (nTarget >= 0)
                        {
                            SetList[nSet].nAction = 3;
                            SetList[nSet].nFrame  = 0;
                            SetList[nSet].nTarget = nTarget;

                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> 1;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                        }
                    }

                    return;
                }

                case 1:
                {
                    if (FindPlayer(nSprite, 1000) >= 0)
                    {
                        SetList[nSet].field_A--;
                        if (SetList[nSet].field_A <= 0)
                        {
                            SetList[nSet].nAction = 2;
                            SetList[nSet].nFrame  = 0;
                        }
                    }

                    return;
                }

                case 2:
                {
                    if (bVal)
                    {
                        SetList[nSet].nAction = 7;
                        SetList[nSet].field_C = 0;
                        SetList[nSet].nFrame  = 0;

                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;

                        SetList[nSet].nTarget = FindPlayer(nSprite, 1000);
                    }
                    return;
                }

                case 3:
                {
                    if (nTarget != -1)
                    {
                        if ((nFlag & 0x10) && (nMov & 0x20000))
                        {
                            SetQuake(nSprite, 100);
                        }

                        int nCourse = PlotCourseToSprite(nSprite, nTarget);

                        if ((nSet & 0x1F) == (totalmoves & 0x1F))
                        {
                            int nRand = RandomSize(3);

                            switch (nRand)
                            {
                                case 0:
                                case 2:
                                {
                                    SetList[nSet].field_C = 0;
                                    SetList[nSet].nAction = 7;
                                    SetList[nSet].nFrame  = 0;
                                    sprite[nSprite].xvel = 0;
                                    sprite[nSprite].yvel = 0;
                                    return;
                                }
                                case 1:
                                {
                                    PlotCourseToSprite(nSprite, nTarget);

                                    SetList[nSet].nAction = 6;
                                    SetList[nSet].nFrame  = 0;
                                    SetList[nSet].field_E = 5;
                                    sprite[nSprite].xvel = 0;
                                    sprite[nSprite].yvel = 0;
                                    return;
                                }
                                default:
                                {
                                    if (nCourse <= 100)
                                    {
                                        SetList[nSet].field_D = 0;
                                    }
                                    else
                                    {
                                        SetList[nSet].field_D = 1;
                                    }
                                    break;
                                }
                            }
                        }

                        // loc_338E2
                        int nAngle = sprite[nSprite].ang & 0xFFF8;
                        sprite[nSprite].xvel = Cos(nAngle) >> 1;
                        sprite[nSprite].yvel = Sin(nAngle) >> 1;

                        if (SetList[nSet].field_D)
                        {
                            sprite[nSprite].xvel *= 2;
                            sprite[nSprite].yvel *= 2;
                        }

                        if ((nMov & 0xC000) == 0x8000)
                        {
                            short nWall = nMov & 0x3FFF;
                            short nSector = wall[nWall].nextsector;

                            if (nSector >= 0)
                            {
                                if ((sprite[nSprite].z - sector[nSector].floorz) < 55000)
                                {
                                    if (sprite[nSprite].z > sector[nSector].ceilingz)
                                    {
                                        SetList[nSet].field_C = 1;
                                        SetList[nSet].nAction = 7;
                                        SetList[nSet].nFrame  = 0;
                                        sprite[nSprite].xvel = 0;
                                        sprite[nSprite].yvel = 0;
                                        return;
                                    }
                                }
                            }

                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> 1;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                            break;
                        }
                        else if ((nMov & 0xC000) == 0xC000)
                        {
                            if (nTarget == (nMov & 0x3FFF))
                            {
                                int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                                if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                                {
                                    SetList[nSet].nAction = 4;
                                    SetList[nSet].nFrame  = 0;
                                }
                                break;
                            }
                            else
                            {
                                SetList[nSet].field_C = 1;
                                SetList[nSet].nAction = 7;
                                SetList[nSet].nFrame  = 0;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                                return;
                            }
                        }

                        break;
                    }
                    else
                    {
                        SetList[nSet].nAction = 0;
                        SetList[nSet].nFrame  = 0;
                        return;
                    }
                }

                case 4:
                {
                    if (nTarget == -1)
                    {
                        SetList[nSet].nAction = 0;
                        SetList[nSet].field_A = 50;
                    }
                    else
                    {
                        if (PlotCourseToSprite(nSprite, nTarget) >= 768)
                        {
                            SetList[nSet].nAction = 3;
                        }
                        else if (nFlag & 0x80)
                        {
                            runlist_DamageEnemy(nTarget, nSprite, 5);
                        }
                    }

                    break;
                }

                case 5:
                {
                    if (bVal)
                    {
                        SetList[nSet].nAction = 0;
                        SetList[nSet].field_A = 15;
                    }
                    return;
                }

                case 6:
                {
                    if (nFlag & 0x80)
                    {
                        // low 16 bits of returned var contains the sprite index, the high 16 the bullet number
                        int nBullet = BuildBullet(nSprite, 11, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1);
                        SetBulletEnemy(FixedToInt(nBullet), nTarget); // isolate the bullet number (shift off the sprite index)

                        SetList[nSet].field_E--;
                        if (SetList[nSet].field_E <= 0 || !RandomBit())
                        {
                            SetList[nSet].nAction = 0;
                            SetList[nSet].nFrame  = 0;
                        }
                    }
                    return;
                }

                case 7:
                {
                    if (bVal)
                    {
                        if (SetList[nSet].field_C)
                        {
                            sprite[nSprite].zvel = -10000;
                        }
                        else
                        {
                            sprite[nSprite].zvel = -(PlotCourseToSprite(nSprite, nTarget));
                        }

                        SetList[nSet].nAction = 8;
                        SetList[nSet].nFrame  = 0;

                        sprite[nSprite].xvel = Cos(sprite[nSprite].ang);
                        sprite[nSprite].yvel = Sin(sprite[nSprite].ang);
                    }
                    return;
                }

                case 8:
                {
                    if (bVal)
                    {
                        SetList[nSet].nFrame = SeqSize[nSeq] - 1;
                    }

                    if (nMov & 0x20000)
                    {
                        SetQuake(nSprite, 200);
                        SetList[nSet].nAction = 9;
                        SetList[nSet].nFrame  = 0;
                    }
                    return;
                }

                case 9:
                {
                    sprite[nSprite].xvel >>= 1;
                    sprite[nSprite].yvel >>= 1;

                    if (bVal)
                    {
                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;

                        PlotCourseToSprite(nSprite, nTarget);

                        SetList[nSet].nAction = 6;
                        SetList[nSet].nFrame  = 0;
                        SetList[nSet].field_E = 5;

                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                    }
                    return;
                }

                case 10:
                {
                    if (nFlag & 0x80)
                    {
                        sprite[nSprite].z -= GetSpriteHeight(nSprite);
                        BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqSet, 76, 0));
                        sprite[nSprite].z += GetSpriteHeight(nSprite);
                    }

                    if (bVal)
                    {
                        SetList[nSet].nAction = 11;
                        SetList[nSet].nFrame  = 0;

                        runlist_ChangeChannel(SetChan[nSet], 1);

                        for (int i = 0; i < 20; i++)
                        {
                            BuildSoul(nSet);
                        }
                    }
                    return;
                }

                case 11:
                {
                    sprite[nSprite].cstat &= 0xFEFE;
                    return;
                }
            }

            // loc_33AE3: ?
            if (nAction)
            {
                if (nTarget != -1)
                {
                    if (!(sprite[nTarget].cstat & 0x101))
                    {
                        SetList[nSet].nAction = 0;
                        SetList[nSet].nFrame  = 0;
                        SetList[nSet].field_A = 100;
                        SetList[nSet].nTarget = -1;
                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                    }
                }
            }

            return;
        }
    }
}
END_PS_NS
