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
#include "sound.h"
#include "player.h"
#include <assert.h>

BEGIN_PS_NS

struct Rex
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nCount;
    short nChannel;
};


TArray<Rex> RexList;

static actionSeq RexSeq[] = {
    {29, 0},
    {0,  0},
    {0,  0},
    {37, 0},
    {9,  0},
    {18, 0},
    {27, 1},
    {28, 1}
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Rex& w, Rex* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("count", w.nCount)
            ("channel", w.nChannel)
            .EndObject();
    }
    return arc;
}

void SerializeRex(FSerializer& arc)
{
    arc("rex", RexList);
}

void InitRexs()
{
    RexList.Clear();
}

void BuildRex(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    int nRex = RexList.Reserve(1);
    auto pActor = &RexList[nRex];
    auto pSprite = &sprite[nSprite];

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 119);
        pSprite = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 119);
        x = pSprite->x;
        y = pSprite->y;
        z = sector[pSprite->sectnum].floorz;
        nAngle = pSprite->ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->clipdist = 80;
    pSprite->shade = -12;
    pSprite->xrepeat = 64;
    pSprite->yrepeat = 64;
    pSprite->picnum = 1;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;
    pSprite->hitag = 0;

    GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 4000;
    pActor->nFrame = 0;
    pActor->nSprite = nSprite;
    pActor->nTarget = -1;
    pActor->nCount = 0;

    pActor->nChannel = nChannel;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nRex, 0x180000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, nRex, 0x180000);

    nCreaturesTotal++;
}

void AIRex::RadialDamage(RunListEvent* ev)
{
    short nRex = RunData[ev->nRun].nObjIndex;
    assert(nRex >= 0 && nRex < (int)RexList.Size());
    auto pActor = &RexList[nRex];

    short nAction = pActor->nAction;
    short nSprite = pActor->nSprite;

    if (nAction == 5)
    {
        ev->nDamage = runlist_CheckRadialDamage(nSprite);
    }
    Damage(ev);
}

void AIRex::Damage(RunListEvent* ev)
{
    short nRex = RunData[ev->nRun].nObjIndex;
    assert(nRex >= 0 && nRex < (int)RexList.Size());
    auto pActor = &RexList[nRex];

    short nAction = pActor->nAction;
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];

    if (ev->nDamage)
    {
        short nTarget = ev->nParam;
        if (nTarget >= 0 && sprite[nTarget].statnum == 100)
        {
            pActor->nTarget = nTarget;
        }

        if (pActor->nAction == 5 && pActor->nHealth > 0)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);

            if (pActor->nHealth <= 0)
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->zvel = 0;
                pSprite->cstat &= 0xFEFE;

                pActor->nHealth = 0;

                nCreaturesKilled++;

                if (nAction < 6)
                {
                    pActor->nAction = 6;
                    pActor->nFrame = 0;
                }
            }
        }
    }
}

void AIRex::Draw(RunListEvent* ev)
{
    short nRex = RunData[ev->nRun].nObjIndex;
    assert(nRex >= 0 && nRex < (int)RexList.Size());
    auto pActor = &RexList[nRex];

    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRex] + RexSeq[nAction].a, pActor->nFrame, RexSeq[nAction].b);
    return;
}

void AIRex::Tick(RunListEvent* ev)
{
    short nRex = RunData[ev->nRun].nObjIndex;
    assert(nRex >= 0 && nRex < (int)RexList.Size());
    auto pActor = &RexList[nRex];

    short nAction = pActor->nAction;
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    Gravity(nSprite);

    int nSeq = SeqOffsets[kSeqRex] + RexSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    int ecx = 2;

    if (nAction != 2) {
        ecx = 1;
    }

    // moves the mouth open and closed as it's idle?
    while (--ecx != -1)
    {
        seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

        pActor->nFrame++;
        if (pActor->nFrame >= SeqSize[nSeq])
        {
            pActor->nFrame = 0;
            bVal = true;
        }
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];

    short nTarget = pActor->nTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if (!pActor->nCount)
        {
            if ((nRex & 0x1F) == (totalmoves & 0x1F))
            {
                if (nTarget < 0)
                {
                    short nAngle = pSprite->ang; // make backup of this variable
                    pActor->nTarget = FindPlayer(nSprite, 60);
                    pSprite->ang = nAngle;
                }
                else
                {
                    pActor->nCount = 60;
                }
            }
        }
        else
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nAction = 1;
                pActor->nFrame = 0;

                pSprite->xvel = bcos(pSprite->ang, -2);
                pSprite->yvel = bsin(pSprite->ang, -2);

                D3PlayFX(StaticSound[kSound48], nSprite);

                pActor->nCount = 30;
            }
        }

        return;
    }

    case 1:
    {
        if (pActor->nCount > 0)
        {
            pActor->nCount--;
        }

        if ((nRex & 0x0F) == (totalmoves & 0x0F))
        {
            if (!RandomSize(1))
            {
                pActor->nAction = 5;
                pActor->nFrame = 0;
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                return;
            }
            else
            {
                if (((PlotCourseToSprite(nSprite, nTarget) >> 8) >= 60) || pActor->nCount > 0)
                {
                    int nAngle = pSprite->ang & 0xFFF8;
                    pSprite->xvel = bcos(nAngle, -2);
                    pSprite->yvel = bsin(nAngle, -2);
                }
                else
                {
                    pActor->nAction = 2;
                    pActor->nCount = 240;
                    D3PlayFX(StaticSound[kSound48], nSprite);
                    pActor->nFrame = 0;
                    return;
                }
            }
        }

        int nMov = MoveCreatureWithCaution(nSprite);

        switch ((nMov & 0xC000))
        {
        case 0xC000:
        {
            if ((nMov & 0x3FFF) == nTarget)
            {
                PlotCourseToSprite(nSprite, nTarget);
                pActor->nAction = 4;
                pActor->nFrame = 0;
                break;
            }
            fallthrough__;
        }
        case 0x8000:
        {
            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -2);
            pSprite->yvel = bsin(pSprite->ang, -2);
            pActor->nAction = 1;
            pActor->nFrame = 0;
            nAction = 1;
            break;
        }
        }

        break;
    }

    case 2:
    {
        pActor->nCount--;
        if (pActor->nCount > 0)
        {
            PlotCourseToSprite(nSprite, nTarget);

            pSprite->xvel = bcos(pSprite->ang, -1);
            pSprite->yvel = bsin(pSprite->ang, -1);

            int nMov = MoveCreatureWithCaution(nSprite);

            switch (nMov & 0xC000)
            {
            case 0x8000:
            {
                SetQuake(nSprite, 25);
                pActor->nCount = 60;

                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -2);
                pSprite->yvel = bsin(pSprite->ang, -2);
                pActor->nAction = 1;
                pActor->nFrame = 0;
                nAction = 1;
                break;
            }
            case 0xC000:
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;

                short nSprite2 = nMov & 0x3FFF;

                if (sprite[nSprite2].statnum && sprite[nSprite2].statnum < 107)
                {
                    short nAngle = pSprite->ang;

                    runlist_DamageEnemy(nSprite2, nSprite, 15);

                    int xVel = bcos(nAngle) * 15;
                    int yVel = bsin(nAngle) * 15;

                    if (sprite[nSprite2].statnum == 100)
                    {
                        short nPlayer = GetPlayerFromSprite(nSprite2);
                        nXDamage[nPlayer] += (xVel << 4);
                        nYDamage[nPlayer] += (yVel << 4);
                        sprite[nSprite2].zvel = -3584;
                    }
                    else
                    {
                        sprite[nSprite2].xvel += (xVel >> 3);
                        sprite[nSprite2].yvel += (yVel >> 3);
                        sprite[nSprite2].zvel = -2880;
                    }
                }

                pActor->nCount >>= 2;
                return;
            }
            }
        }
        else
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 90;
        }

        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nAction = 2;
        }
        return;
    }

    case 4:
    {
        if (nTarget != -1)
        {
            if (PlotCourseToSprite(nSprite, nTarget) < 768)
            {
                if (nFlag & 0x80)
                {
                    runlist_DamageEnemy(nTarget, nSprite, 15);
                }

                break;
            }
        }

        pActor->nAction = 1;
        break;
    }

    case 5:
    {
        if (bVal)
        {
            pActor->nAction = 1;
            pActor->nCount = 15;
        }
        return;
    }

    case 6:
    {
        if (bVal)
        {
            pActor->nAction = 7;
            pActor->nFrame = 0;
            runlist_ChangeChannel(pActor->nChannel, 1);
        }
        return;
    }

    case 7:
    {
        pSprite->cstat &= 0xFEFE;
        return;
    }
    }

    // break-ed
    if (nAction > 0)
    {
        if ((nTarget != -1) && (!(sprite[nTarget].cstat & 0x101)))
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->nCount = 0;
            pActor->nTarget = -1;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }
    return;
}


void FuncRex(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRex ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}
END_PS_NS
