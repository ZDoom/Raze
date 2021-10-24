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
    auto pSprite = &sprite[nSprite];
    pSprite->xvel = bcos(pSprite->ang);
    pSprite->yvel = bsin(pSprite->ang);
}

int BuildWasp(short nSprite, int x, int y, int z, short nSector, short nAngle)
{
    auto nWasp = WaspList.Reserve(1);
    auto pActor = &WaspList[nWasp];

    uint8_t bEggWasp = false;
    if (nSprite == -2) {
        bEggWasp = true;
    }
    auto pSprite = &sprite[nSprite];

    if (nSprite < 0)
    {
        nSprite = insertsprite(nSector, 107);
        assert(nSprite >= 0 && nSprite < kMaxSprites);
        pSprite = &sprite[nSprite];

        pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z;
    }
    else
    {
        nAngle = pSprite->ang;
        changespritestat(nSprite, 107);
    }

    pSprite->shade = -12;
    pSprite->cstat = 0x101;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->clipdist = 70;

    if (bEggWasp)
    {
        pSprite->xrepeat = 20;
        pSprite->yrepeat = 20;
    }
    else
    {
        pSprite->xrepeat = 50;
        pSprite->yrepeat = 50;
    }

    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->ang = nAngle;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

    //	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nFrame = 0;
    pActor->nSprite = nSprite;
    pActor->nTarget = -1;
    pActor->nHealth = 800;
    pActor->nDamage = 10;

    if (bEggWasp)
    {
        pActor->nCount = 60;
        pActor->nDamage /= 2;
    }
    else
    {
        pActor->nCount = RandomSize(5);
    }

    pActor->nAngle = 0;
    pActor->nVel = 0;
    pActor->nAngle2 = RandomSize(7) + 127;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nWasp, 0x1E0000);

    pActor->nRun = runlist_AddRunRec(NewRun, nWasp, 0x1E0000);

    nCreaturesTotal++;
    return nSprite;
}

void AIWasp::Draw(RunListEvent* ev)
{
    short nWasp = RunData[ev->nRun].nObjIndex;
    auto pActor = &WaspList[nWasp];
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqWasp] + WaspSeq[nAction].a, pActor->nFrame, WaspSeq[nAction].b);
    return;
}

void AIWasp::RadialDamage(RunListEvent* ev)
{
    short nWasp = RunData[ev->nRun].nObjIndex;
    auto pActor = &WaspList[nWasp];
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];

    if (!(pSprite->cstat & 0x101))
        return;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    Damage(ev);
}

void AIWasp::Damage(RunListEvent* ev)
{
    short nWasp = RunData[ev->nRun].nObjIndex;
    auto pActor = &WaspList[nWasp];
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pActor->nAction;

    if (!ev->nDamage) {
        return;
    }

    if (pActor->nHealth > 0)
    {
        pActor->nHealth -= dmgAdjust(ev->nDamage, 3);

        if (pActor->nHealth > 0)
        {
            if (!RandomSize(4))
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;
            }

            pActor->nAction = 1;
            pSprite->ang += RandomSize(9) + 768;
            pSprite->ang &= kAngleMask;

            pActor->nVel = 3000;

            pSprite->zvel = (-20) - RandomSize(6);
        }
        else
        {
            // Wasp is dead
            pActor->nAction = 4;
            pActor->nFrame = 0;

            pSprite->cstat = 0;
            pSprite->ang = (pSprite->ang + 1024) & kAngleMask;

            SetWaspVel(nSprite);

            pSprite->zvel = 512;

            nCreaturesKilled++;
        }
    }
    return;
}

void AIWasp::Tick(RunListEvent* ev)
{
    short nWasp = RunData[ev->nRun].nObjIndex;
    auto pActor = &WaspList[nWasp];
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pActor->nAction;

    short nTarget = -1;

    bool bVal = false;

    short nSeq = SeqOffsets[kSeqWasp] + WaspSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    if (pActor->nHealth > 0)
    {
        nTarget = pActor->nTarget;

        if (nTarget > -1 && (!(sprite[nTarget].cstat & 0x101) || (SectFlag[sprite[nTarget].sectnum] & kSectUnderwater)))
        {
            // goto pink
            pActor->nTarget = -1;
            pActor->nAction = 0;
            pActor->nCount = RandomSize(6);
            return;
        }
    }

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        pSprite->zvel = bsin(pActor->nAngle, -4);

        pActor->nAngle += pActor->nAngle2;
        pActor->nAngle &= kAngleMask;

        MoveCreature(nSprite);

        if (nTarget >= 0)
        {
            pActor->nCount--;
            if (pActor->nCount > 0)
            {
                PlotCourseToSprite(nSprite, nTarget);
            }
            else
            {
                pSprite->zvel = 0;
                pActor->nAction = 1;
                pActor->nFrame = 0;
                pActor->nVel = 1500;
                pActor->nCount = RandomSize(5) + 60;
            }
        }
        else
        {
            if ((nWasp & 0x1F) == (totalmoves & 0x1F)) {
                pActor->nTarget = FindPlayer(nSprite, 60);
            }
        }

        return;
    }

    case 1:
    {
        pActor->nCount--;

        if (pActor->nCount <= 0)
        {
            pActor->nAction = 0;
            pActor->nCount = RandomSize(6);
            return;
        }

        int nChaseVal = AngleChase(nSprite, nTarget, pActor->nVel, 0, 16);

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
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                runlist_DamageEnemy(nSprite2, nSprite, pActor->nDamage);
                pActor->nAction = 2;
                pActor->nFrame = 0;
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
            pSprite->ang += RandomSize(9) + 768;
            pSprite->ang &= kAngleMask;
            pSprite->zvel = (-20) - RandomSize(6);

            pActor->nAction = 1;
            pActor->nVel = 3000;
        }
        return;
    }
    case 4:
    {
        int nMove = MoveCreature(nSprite) & 0x8000;
        nMove |= 0xC000;

        if (nMove)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 1024;
            pActor->nAction = 5;
            pActor->nFrame = 0;
        }

        return;
    }
    case 5:
    {
        short nSector = pSprite->sectnum;

        pSprite->z += pSprite->zvel;

        if (pSprite->z >= sector[nSector].floorz)
        {
            if (SectBelow[nSector] > -1)
            {
                BuildSplash(nSprite, nSector);
                pSprite->cstat |= 0x8000;
            }

            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
            runlist_SubRunRec(pActor->nRun);
        }

        return;
    }
    }
}

void FuncWasp(int nObject, int nMessage, int nDamage, int nRun)
{
    AIWasp ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

END_PS_NS
