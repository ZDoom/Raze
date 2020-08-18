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
#include "random.h"
#include "exhumed.h"
#include "sequence.h"
#include "init.h"
#include "trigdat.h"
#include <assert.h>

BEGIN_PS_NS

#define kMaxWasps	100

static short nVelShift = 0;
short nWaspCount;

struct Wasp
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nRun;
    short nTarget;
    short field_C;
    short field_E;
    short field_10;
    short field_12;
    short field_14;
    short nDamage;
};

Wasp WaspList[kMaxWasps];

static actionSeq ActionSeq[] = {
    {0,  0},
    {0,  0},
    {9,  0},
    {18, 0},
    {27, 1},
    {28, 1},
    {29, 1}
};

static SavegameHelper sgh("wasp",
    SV(nVelShift),
    SV(nWaspCount),
    SA(WaspList),
    nullptr);


void InitWasps()
{
    nWaspCount = 0;
}

void SetWaspVel(short nSprite)
{
    if (nVelShift < 0)
    {
        sprite[nSprite].xvel = Cos(sprite[nSprite].ang) << -nVelShift;
        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) << -nVelShift;
    }
    else
    {
        sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> nVelShift;
        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> nVelShift;
    }
}

int BuildWasp(short nSprite, int x, int y, int z, short nSector, short nAngle)
{
    if (nWaspCount >= kMaxWasps) {
        return -1;
    }

    short nWasp = nWaspCount;
    nWaspCount++;

    uint8_t bEggWasp = kFalse;
    if (nSprite == -2) {
        bEggWasp = kTrue;
    }

    if (nSprite < 0)
    {
        nSprite = insertsprite(nSector, 107);
        assert(nSprite >= 0 && nSprite < kMaxSprites);

        sprite[nSprite].x = x;
        sprite[nSprite].y = y;
        sprite[nSprite].z = z;
    }
    else
    {
        nAngle = sprite[nSprite].ang;
        changespritestat(nSprite, 107);
    }

    sprite[nSprite].shade = -12;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].clipdist = 70;

    if (bEggWasp)
    {
        sprite[nSprite].xrepeat = 20;
        sprite[nSprite].yrepeat = 20;
    }
    else
    {
        sprite[nSprite].xrepeat = 50;
        sprite[nSprite].yrepeat = 50;
    }

    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

    WaspList[nWasp].nAction = 0;
    WaspList[nWasp].nFrame  = 0;
    WaspList[nWasp].nSprite = nSprite;
    WaspList[nWasp].nTarget = -1;
    WaspList[nWasp].nHealth = 800;
    WaspList[nWasp].nDamage = 10;

    if (bEggWasp)
    {
        WaspList[nWasp].field_C = 60;
        WaspList[nWasp].nDamage /= 2;
    }
    else
    {
        WaspList[nWasp].field_C = RandomSize(5);
    }

    WaspList[nWasp].field_E = 0;
    WaspList[nWasp].field_14 = 0;
    WaspList[nWasp].field_12 = 0;
    WaspList[nWasp].field_10 = RandomSize(7) + 127;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nWasp | 0x1E0000);

    WaspList[nWasp].nRun = runlist_AddRunRec(NewRun, nWasp | 0x1E0000);

    nCreaturesLeft++;
    return nSprite;
}

void FuncWasp(int a, int nDamage, int nRun)
{
    short nWasp = RunData[nRun].nVal;
    short nSprite = WaspList[nWasp].nSprite;
    short nAction = WaspList[nWasp].nAction;

    short nTarget = -1;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqWasp] + ActionSeq[nAction].a, WaspList[nWasp].nFrame, ActionSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            if (!(sprite[nSprite].cstat & 0x101))
                return;

            nDamage = runlist_CheckRadialDamage(nSprite);
            // fall through to case 0x80000
            fallthrough__;
        }

        case 0x80000:
        {
            if (!nDamage) {
                return;
            }

            if (WaspList[nWasp].nHealth > 0)
            {
                WaspList[nWasp].nHealth -= nDamage;

                if (WaspList[nWasp].nHealth > 0)
                {
                    if (!RandomSize(4))
                    {
                        WaspList[nWasp].nAction = 3;
                        WaspList[nWasp].nFrame  = 0;
                    }

                    WaspList[nWasp].nAction = 1;
                    sprite[nSprite].ang += RandomSize(9) + 768;
                    sprite[nSprite].ang &= kAngleMask;

                    WaspList[nWasp].field_12 = 3000;

                    sprite[nSprite].zvel = (-20) - RandomSize(6);
                }
                else
                {
                    // Wasp is dead
                    nVelShift = 0;

                    WaspList[nWasp].nAction = 4;
                    WaspList[nWasp].nFrame  = 0;

                    sprite[nSprite].cstat = 0;
                    sprite[nSprite].ang = (sprite[nSprite].ang + 1024) & kAngleMask;

                    SetWaspVel(nSprite);

                    sprite[nSprite].zvel = 512;

                    nCreaturesLeft--;
                }
            }
            return;
        }

        case 0x20000:
        {
            short nSeq = SeqOffsets[kSeqWasp] + ActionSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, WaspList[nWasp].nFrame);

            seq_MoveSequence(nSprite, nSeq, WaspList[nWasp].nFrame);

            WaspList[nWasp].nFrame++;
            if (WaspList[nWasp].nFrame >= SeqSize[nSeq])
            {
                WaspList[nWasp].nFrame = 0;
                bVal = true;
            }

            if (WaspList[nWasp].nHealth > 0)
            {
                nTarget = WaspList[nWasp].nTarget;

                if (nTarget > -1 && (!(sprite[nTarget].cstat & 0x101) || (SectFlag[sprite[nTarget].sectnum] & kSectUnderwater)))
                {
                    // goto pink
                    WaspList[nWasp].nTarget = -1;
                    WaspList[nWasp].nAction = 0;
                    WaspList[nWasp].field_C = RandomSize(6);
                    return;
                }
            }

            switch (nAction)
            {
                default:
                    return;

                case 0:
                {
                    sprite[nSprite].zvel = Sin(WaspList[nWasp].field_E) >> 4;

                    WaspList[nWasp].field_E += WaspList[nWasp].field_10;
                    WaspList[nWasp].field_E &= kAngleMask;

                    MoveCreature(nSprite);

                    if (nTarget >= 0)
                    {
                        WaspList[nWasp].field_C--;
                        if (WaspList[nWasp].field_C > 0)
                        {
                            PlotCourseToSprite(nSprite, nTarget);
                        }
                        else
                        {
                            sprite[nSprite].zvel = 0;
                            WaspList[nWasp].nAction = 1;
                            WaspList[nWasp].nFrame  = 0;
                            WaspList[nWasp].field_12 = 1500;
                            WaspList[nWasp].field_C = RandomSize(5) + 60;
                        }
                    }
                    else
                    {
                        if ((nWasp & 0x1F) == (totalmoves & 0x1F)) {
                            WaspList[nWasp].nTarget = FindPlayer(nSprite, 60);
                        }
                    }

                    return;
                }

                case 1:
                {
                    WaspList[nWasp].field_C--;

                    if (WaspList[nWasp].field_C <= 0)
                    {
                        WaspList[nWasp].nAction = 0;
                        WaspList[nWasp].field_C = RandomSize(6);
                        return;
                    }

                    int nChaseVal = AngleChase(nSprite, nTarget, WaspList[nWasp].field_12, 0, 16);

                    switch (nChaseVal & 0xC000)
                    {
                        default:
                            return;

                        case 0x8000:
                        {
                            return;
                        }

                        case 0xC000:
                        {
                            short nSprite2 = (nChaseVal & 0x3FFF);
                            if (nSprite2 == nTarget)
                            {
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                                runlist_DamageEnemy(nSprite2, nSprite, WaspList[nWasp].nDamage);
                                WaspList[nWasp].nAction = 2;
                                WaspList[nWasp].nFrame = 0;
                            }
                            return;
                        }
                    }

                    return;
                }

                case 2:
                case 3:
                {
                    if (bVal)
                    {
                        sprite[nSprite].ang += RandomSize(9) + 768;
                        sprite[nSprite].ang &= kAngleMask;
                        sprite[nSprite].zvel = (-20) - RandomSize(6);

                        WaspList[nWasp].nAction = 1; 
                        WaspList[nWasp].field_12 = 3000;
                    }
                    return;
                }
                case 4:
                {
                    int nMove = MoveCreature(nSprite) & 0x8000;
                    nMove |= 0xC000;

                    if (nMove)
                    {
                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                        sprite[nSprite].zvel = 1024;
                        WaspList[nWasp].nAction = 5;
                        WaspList[nWasp].nFrame = 0;
                    }

                    return;
                }
                case 5:
                {
                    short nSector = sprite[nSprite].sectnum;

                    sprite[nSprite].z += sprite[nSprite].zvel;

                    if (sprite[nSprite].z >= sector[nSector].floorz)
                    {
                        if (SectBelow[nSector] > -1)
                        {
                            BuildSplash(nSprite, nSector);
                            sprite[nSprite].cstat |= 0x8000;
                        }

                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                        sprite[nSprite].zvel = 0;
                        WaspList[nWasp].nAction = 6;
                        WaspList[nWasp].nFrame = 0;
                        runlist_SubRunRec(WaspList[nWasp].nRun);
                    }

                    return;
                }
                }

            break;
        }
    }
}
END_PS_NS
