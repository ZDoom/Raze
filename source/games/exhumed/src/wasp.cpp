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

struct Wasp
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nRun;
    short nTarget;
    short nCount;
    short nAngle;
    short nAngle2;
    short nVel;
    short nDamage;
};

TArray<Wasp> WaspList;

static actionSeq WaspSeq[] = {
    {0,  0},
    {0,  0},
    {9,  0},
    {18, 0},
    {27, 1},
    {28, 1},
    {29, 1}
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Wasp& w, Wasp* def)
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
            ("angle", w.nAngle)
            ("angle2", w.nAngle2)
            ("vel", w.nVel)
            ("damage", w.nDamage)
            .EndObject();
    }
    return arc;
}

void SerializeWasp(FSerializer& arc)
{
    arc("wasp", WaspList);
}

int WaspCount()
{
    return WaspList.Size();
}

void InitWasps()
{
    WaspList.Clear();
}

void SetWaspVel(short nSprite)
{
    sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
    sprite[nSprite].yvel = bsin(sprite[nSprite].ang);
}

int BuildWasp(short nSprite, int x, int y, int z, short nSector, short nAngle)
{
    auto nWasp = WaspList.Reserve(1);

    uint8_t bEggWasp = false;
    if (nSprite == -2) {
        bEggWasp = true;
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
        WaspList[nWasp].nCount = 60;
        WaspList[nWasp].nDamage /= 2;
    }
    else
    {
        WaspList[nWasp].nCount = RandomSize(5);
    }

    WaspList[nWasp].nAngle = 0;
    WaspList[nWasp].nVel = 0;
    WaspList[nWasp].nAngle2 = RandomSize(7) + 127;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nWasp | 0x1E0000);

    WaspList[nWasp].nRun = runlist_AddRunRec(NewRun, nWasp | 0x1E0000);

    nCreaturesTotal++;
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
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqWasp] + WaspSeq[nAction].a, WaspList[nWasp].nFrame, WaspSeq[nAction].b);
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

                    WaspList[nWasp].nVel = 3000;

                    sprite[nSprite].zvel = (-20) - RandomSize(6);
                }
                else
                {
                    // Wasp is dead
                    WaspList[nWasp].nAction = 4;
                    WaspList[nWasp].nFrame  = 0;

                    sprite[nSprite].cstat = 0;
                    sprite[nSprite].ang = (sprite[nSprite].ang + 1024) & kAngleMask;

                    SetWaspVel(nSprite);

                    sprite[nSprite].zvel = 512;

                    nCreaturesKilled++;
                }
            }
            return;
        }

        case 0x20000:
        {
            short nSeq = SeqOffsets[kSeqWasp] + WaspSeq[nAction].a;

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
                    WaspList[nWasp].nCount = RandomSize(6);
                    return;
                }
            }

            switch (nAction)
            {
                default:
                    return;

                case 0:
                {
                    sprite[nSprite].zvel = bsin(WaspList[nWasp].nAngle, -4);

                    WaspList[nWasp].nAngle += WaspList[nWasp].nAngle2;
                    WaspList[nWasp].nAngle &= kAngleMask;

                    MoveCreature(nSprite);

                    if (nTarget >= 0)
                    {
                        WaspList[nWasp].nCount--;
                        if (WaspList[nWasp].nCount > 0)
                        {
                            PlotCourseToSprite(nSprite, nTarget);
                        }
                        else
                        {
                            sprite[nSprite].zvel = 0;
                            WaspList[nWasp].nAction = 1;
                            WaspList[nWasp].nFrame  = 0;
                            WaspList[nWasp].nVel = 1500;
                            WaspList[nWasp].nCount = RandomSize(5) + 60;
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
                    WaspList[nWasp].nCount--;

                    if (WaspList[nWasp].nCount <= 0)
                    {
                        WaspList[nWasp].nAction = 0;
                        WaspList[nWasp].nCount = RandomSize(6);
                        return;
                    }

                    int nChaseVal = AngleChase(nSprite, nTarget, WaspList[nWasp].nVel, 0, 16);

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
                        WaspList[nWasp].nVel = 3000;
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
