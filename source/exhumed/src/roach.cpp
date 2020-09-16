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
#include "exhumed.h"
#include "engine.h"
#include "sequence.h"
#include <assert.h>

BEGIN_PS_NS

#define kMaxRoach	100

int16_t RoachCount = -1;

static actionSeq ActionSeq[] = {
    {24, 0},
    {0,  0},
    {0,  0},
    {16, 0},
    {8,  0},
    {32, 1},
    {42, 1}
};

struct Roach
{
    short nHealth;
    short field_2;
    short nAction;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short field_E;
};

Roach RoachList[kMaxRoach];

static SavegameHelper sgh("roach",
    SV(RoachCount),
    SA(RoachList),
    nullptr);


/* Kilmaat Sentry */

void InitRoachs()
{
    RoachCount = kMaxRoach;
}

// TODO - make nType a bool?
int BuildRoach(int nType, int nSprite, int x, int y, int z, short nSector, int angle)
{
    RoachCount--;
    if (RoachCount < 0) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 105);
    }
    else
    {
        changespritestat(nSprite, 105);
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sector[sprite[nSprite].sectnum].floorz;
        angle = sprite[nSprite].ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].shade = -12;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].clipdist = 60;
    sprite[nSprite].ang = angle;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;

    //	GrabTimeSlot(3);

    if (nType)
    {
        RoachList[RoachCount].nAction = 0;
    }
    else
    {
        RoachList[RoachCount].nAction = 1;
    }

    RoachList[RoachCount].nSprite = nSprite;
    RoachList[RoachCount].field_2 = 0;
    RoachList[RoachCount].field_C = 0;
    RoachList[RoachCount].nTarget = -1;
    RoachList[RoachCount].nHealth = 600;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, RoachCount | 0x1C0000);
    RoachList[RoachCount].field_A = runlist_AddRunRec(NewRun, RoachCount | 0x1C0000);

    nCreaturesTotal++;

    return RoachCount | 0x1C0000;
}

void GoRoach(short nSprite)
{
    sprite[nSprite].xvel = (Cos(sprite[nSprite].ang) >> 1) - (Cos(sprite[nSprite].ang) >> 3);
    sprite[nSprite].yvel = (Sin(sprite[nSprite].ang) >> 1) - (Sin(sprite[nSprite].ang) >> 3);
}

void FuncRoach(int a, int nDamage, int nRun)
{
    short nRoach = RunData[nRun].nVal;
    assert(nRoach >= 0 && nRoach < kMaxRoach);
    
    short nSprite = RoachList[nRoach].nSprite;
    short nAction = RoachList[nRoach].nAction;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Roach\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, ActionSeq[nAction].a + SeqOffsets[kSeqRoach], RoachList[nRoach].field_2, ActionSeq[nAction].b);
            return;
        }

        case 0xA0000: // fall through to next case
        {
            nDamage = runlist_CheckRadialDamage(nSprite);
            fallthrough__;
        }
        case 0x80000:
        {
            if (nDamage)
            {
                if (RoachList[nRoach].nHealth <= 0) {
                    return;
                }

                RoachList[nRoach].nHealth -= nDamage;
                if (RoachList[nRoach].nHealth <= 0)
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                    sprite[nSprite].cstat &= 0xFEFE;

                    RoachList[nRoach].nHealth = 0;

                    if (nAction < 5)
                    {
                        DropMagic(nSprite);
                        RoachList[nRoach].nAction = 5;
                        RoachList[nRoach].field_2 = 0;
                    }

                    nCreaturesKilled++; // NOTE: This was incrementing in original code. Bug?
                }
                else
                {
                    short nSprite2 = a & 0xFFFF;
                    if (nSprite2 >= 0)
                    {
                        if (sprite[nSprite2].statnum < 199) {
                            RoachList[nRoach].nTarget = nSprite2;
                        }

                        if (nAction == 0)
                        {
                            RoachList[nRoach].nAction = 2;
                            GoRoach(nSprite);
                            RoachList[nRoach].field_2 = 0;
                        }
                        else
                        {
                            if (!RandomSize(4))
                            {
                                RoachList[nRoach].nAction = 4;
                                RoachList[nRoach].field_2 = 0;
                            }
                        }
                    }
                }
            }

            return;
        }

        case 0x20000:
        {
            Gravity(nSprite);

            int nSeq = SeqOffsets[kSeqRoach] + ActionSeq[RoachList[nRoach].nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, RoachList[nRoach].field_2);
            seq_MoveSequence(nSprite, nSeq, RoachList[nRoach].field_2);

            RoachList[nRoach].field_2++;
            if (RoachList[nRoach].field_2 >= SeqSize[nSeq])
            {
                bVal = true;
                RoachList[nRoach].field_2 = 0;
            }

            int nFlag = FrameFlag[SeqBase[nSeq] + RoachList[nRoach].field_2];
            short nTarget = RoachList[nRoach].nTarget;

            if (nAction > 5) {
                return;
            }

            switch (nAction)
            {
                case 0:
                {
                    if (RoachList[nRoach].field_2 == 1)
                    {
                        RoachList[nRoach].field_C--;
                        if (RoachList[nRoach].field_C <= 0)
                        {
                            RoachList[nRoach].field_C = RandomSize(6);
                        }
                        else
                        {
                            RoachList[nRoach].field_2 = 0;
                        }
                    }

                    if (((nRoach & 0xF) == (totalmoves & 0xF)) && nTarget < 0)
                    {
                        short nTarget = FindPlayer(nSprite, 50);
                        if (nTarget >= 0)
                        {
                            RoachList[nRoach].nAction = 2;
                            RoachList[nRoach].field_2 = 0;
                            RoachList[nRoach].nTarget = nTarget;
                            GoRoach(nSprite);
                        }
                    }

                    return;
                }

                case 1:
                {
                    // partly the same as case 0.
                    if (((nRoach & 0xF) == (totalmoves & 0xF)) && nTarget < 0)
                    {
                        short nTarget = FindPlayer(nSprite, 100);
                        if (nTarget >= 0)
                        {
                            RoachList[nRoach].nAction = 2;
                            RoachList[nRoach].field_2 = 0;
                            RoachList[nRoach].nTarget = nTarget;
                            GoRoach(nSprite);
                        }
                    }

                    return;
                }

                case 2:
                {
                    if ((totalmoves & 0xF) == (nRoach & 0xF))
                    {
                        PlotCourseToSprite(nSprite, nTarget);
                        GoRoach(nSprite);
                    }

                    int nMov = MoveCreatureWithCaution(nSprite);

                    if ((nMov & 0xC000) == 0xC000)
                    {
                        if ((nMov & 0x3FFF) == nTarget)
                        {
                            // repeated below
                            RoachList[nRoach].field_E = RandomSize(2) + 1;
                            RoachList[nRoach].nAction = 3;

                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
                            sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                            RoachList[nRoach].field_2 = 0;
                        }
                        else
                        {
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            GoRoach(nSprite);
                        }
                    }
                    else if ((nMov & 0xC000) == 0x8000)
                    {
                        sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                        GoRoach(nSprite);
                    }
                    else
                    {
                        if (RoachList[nRoach].field_C != 0)
                        {
                            RoachList[nRoach].field_C--;
                        }
                        else
                        {
                            // same as above
                            RoachList[nRoach].field_E = RandomSize(2) + 1;
                            RoachList[nRoach].nAction = 3;

                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
                            sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                            RoachList[nRoach].field_2 = 0;
                        }
                    }

                    if (nTarget != -1 && !(sprite[nTarget].cstat & 0x101))
                    {
                        RoachList[nRoach].nAction = 1;
                        RoachList[nRoach].field_2 = 0;
                        RoachList[nRoach].field_C = 100;
                        RoachList[nRoach].nTarget = -1;
                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                    }

                    return;
                }

                case 3:
                {
                    if (bVal)
                    {
                        RoachList[nRoach].field_E--;
                        if (RoachList[nRoach].field_E <= 0)
                        {
                            RoachList[nRoach].nAction = 2;
                            GoRoach(nSprite);
                            RoachList[nRoach].field_2 = 0;
                            RoachList[nRoach].field_C = RandomSize(7);
                        }
                    }
                    else
                    {
                        if (nFlag & 0x80)
                        {
                            BuildBullet(nSprite, 13, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1);
                        }
                    }

                    return;
                }

                case 4:
                {
                    if (bVal)
                    {
                        RoachList[nRoach].nAction = 2;
                        RoachList[nRoach].field_2 = 0;
                    }

                    return;
                }

                case 5:
                {
                    if (bVal)
                    {
                        sprite[nSprite].cstat = 0;
                        RoachList[nRoach].nAction = 6;
                        RoachList[nRoach].field_2 = 0;
                    }

                    return;
                }
            }
        }
    }
}
END_PS_NS
