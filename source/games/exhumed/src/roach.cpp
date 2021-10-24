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

static actionSeq RoachSeq[] = {
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
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nRun;
    short nCount;
    short nIndex;
};

TArray<Roach> RoachList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Roach& w, Roach* def)
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
            ("index", w.nIndex)
            .EndObject();
    }
    return arc;
}

void SerializeRoach(FSerializer& arc)
{
    arc("roach", RoachList);
}



/* Kilmaat Sentry */

void InitRoachs()
{
    RoachList.Clear();
}

// TODO - make nType a bool?
void BuildRoach(int nType, int nSprite, int x, int y, int z, short nSector, int angle)
{
    auto RoachCount = RoachList.Reserve(1);

    auto pSprite = &sprite[nSprite];
    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 105);
        pSprite = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 105);
        x = pSprite->x;
        y = pSprite->y;
        z = sector[pSprite->sectnum].floorz;
        angle = pSprite->ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->clipdist = 60;
    pSprite->ang = angle;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

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
    RoachList[RoachCount].nFrame = 0;
    RoachList[RoachCount].nCount = 0;
    RoachList[RoachCount].nTarget = -1;
    RoachList[RoachCount].nHealth = 600;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, RoachCount, 0x1C0000);
    RoachList[RoachCount].nRun = runlist_AddRunRec(NewRun, RoachCount, 0x1C0000);

    nCreaturesTotal++;
}

void GoRoach(short nSprite)
{
    auto pSprite = &sprite[nSprite];
    pSprite->xvel = bcos(pSprite->ang, -1) - bcos(pSprite->ang, -3);
    pSprite->yvel = bsin(pSprite->ang, -1) - bsin(pSprite->ang, -3);
}

void AIRoach::Draw(RunListEvent* ev)
{
    short nRoach = RunData[ev->nRun].nObjIndex;
    assert(nRoach >= 0 && nRoach < (int)RoachList.Size());
    auto pActor = &RoachList[nRoach];
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, RoachSeq[nAction].a + SeqOffsets[kSeqRoach], pActor->nFrame, RoachSeq[nAction].b);
    return;
}

void AIRoach::RadialDamage(RunListEvent* ev)
{
    short nRoach = RunData[ev->nRun].nObjIndex;
    assert(nRoach >= 0 && nRoach < (int)RoachList.Size());
    auto pActor = &RoachList[nRoach];
    short nSprite = pActor->nSprite;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    Damage(ev);
}

void AIRoach::Damage(RunListEvent* ev)
{
    short nRoach = RunData[ev->nRun].nObjIndex;
    assert(nRoach >= 0 && nRoach < (int)RoachList.Size());
    auto pActor = &RoachList[nRoach];

    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pActor->nAction;

    if (ev->nDamage)
    {
        if (pActor->nHealth <= 0) {
            return;
        }

        pActor->nHealth -= dmgAdjust(ev->nDamage);
        if (pActor->nHealth <= 0)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->cstat &= 0xFEFE;

            pActor->nHealth = 0;

            if (nAction < 5)
            {
                DropMagic(nSprite);
                pActor->nAction = 5;
                pActor->nFrame = 0;
            }

            nCreaturesKilled++; // NOTE: This was incrementing in original code. Bug?
        }
        else
        {
            short nSprite2 = ev->nParam;
            if (nSprite2 >= 0)
            {
                if (sprite[nSprite2].statnum < 199) {
                    pActor->nTarget = nSprite2;
                }

                if (nAction == 0)
                {
                    pActor->nAction = 2;
                    GoRoach(nSprite);
                    pActor->nFrame = 0;
                }
                else
                {
                    if (!RandomSize(4))
                    {
                        pActor->nAction = 4;
                        pActor->nFrame = 0;
                    }
                }
            }
        }
    }
}

void AIRoach::Tick(RunListEvent* ev)
{
    short nRoach = RunData[ev->nRun].nObjIndex;
    assert(nRoach >= 0 && nRoach < (int)RoachList.Size());
    auto pActor = &RoachList[nRoach];

    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pActor->nAction;
    bool bVal = false;

    Gravity(nSprite);

    int nSeq = SeqOffsets[kSeqRoach] + RoachSeq[pActor->nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        bVal = true;
        pActor->nFrame = 0;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    short nTarget = pActor->nTarget;

    if (nAction > 5) {
        return;
    }

    switch (nAction)
    {
    case 0:
    {
        if (pActor->nFrame == 1)
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nCount = RandomSize(6);
            }
            else
            {
                pActor->nFrame = 0;
            }
        }

        if (((nRoach & 0xF) == (totalmoves & 0xF)) && nTarget < 0)
        {
            short nTarget = FindPlayer(nSprite, 50);
            if (nTarget >= 0)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->nTarget = nTarget;
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
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->nTarget = nTarget;
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
                pActor->nIndex = RandomSize(2) + 1;
                pActor->nAction = 3;

                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);

                pActor->nFrame = 0;
            }
            else
            {
                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                GoRoach(nSprite);
            }
        }
        else if ((nMov & 0xC000) == 0x8000)
        {
            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            GoRoach(nSprite);
        }
        else
        {
            if (pActor->nCount != 0)
            {
                pActor->nCount--;
            }
            else
            {
                // same as above
                pActor->nIndex = RandomSize(2) + 1;
                pActor->nAction = 3;

                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);

                pActor->nFrame = 0;
            }
        }

        if (nTarget != -1 && !(sprite[nTarget].cstat & 0x101))
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 100;
            pActor->nTarget = -1;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }

        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nIndex--;
            if (pActor->nIndex <= 0)
            {
                pActor->nAction = 2;
                GoRoach(nSprite);
                pActor->nFrame = 0;
                pActor->nCount = RandomSize(7);
            }
        }
        else
        {
            if (nFlag & 0x80)
            {
                BuildBullet(nSprite, 13, 0, 0, -1, pSprite->ang, nTarget + 10000, 1);
            }
        }

        return;
    }

    case 4:
    {
        if (bVal)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
        }

        return;
    }

    case 5:
    {
        if (bVal)
        {
            pSprite->cstat = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
        }

        return;
    }
    }
}

void FuncRoach(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRoach ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
