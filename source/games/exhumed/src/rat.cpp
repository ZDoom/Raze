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
#include "view.h"
#include "exhumed.h"
#include <assert.h>

BEGIN_PS_NS

short nMinChunk;
short nPlayerPic;
short nMaxChunk;

struct Rat
{
    short nFrame;
    short nAction;
    short nSprite;
    short nRun;
    short nTarget;
    short nCount;
    short nIndex;
};


TArray<Rat> RatList;

static actionSeq RatSeq[] = {
    {0, 1},
    {1, 0},
    {1, 0},
    {9, 1},
    {0, 1}
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Rat& w, Rat* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("run", w.nRun)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("count", w.nCount)
            ("index", w.nIndex)
            .EndObject();
    }
    return arc;
}

void SerializeRat(FSerializer& arc)
{
    if (arc.BeginObject("rat"))
    {
        arc("minchunk", nMinChunk)
            ("maxchunk", nMaxChunk)
            ("playerpic", nPlayerPic)
            ("list", RatList)
            .EndObject();
    }
}

void InitRats()
{
    RatList.Clear();
    nMinChunk = 9999;
    nMaxChunk = -1;

    for (int i = 122; i <= 131; i++)
    {
        int nPic = seq_GetSeqPicnum(kSeqJoe, i, 0);

        if (nPic < nMinChunk)
            nMinChunk = nPic;

        if (nPic > nMaxChunk)
            nMaxChunk = nPic;
    }

    nPlayerPic = seq_GetSeqPicnum(kSeqJoe, 120, 0);
}

void SetRatVel(short nSprite)
{
	auto pSprite = &sprite[nSprite];

    pSprite->xvel = bcos(pSprite->ang, -2);
    pSprite->yvel = bsin(pSprite->ang, -2);
}

void BuildRat(short nSprite, int x, int y, int z, short nSector, int nAngle)
{
    auto nRat = RatList.Reserve(1);

	auto pSprite = &sprite[nSprite];

    if (nSprite < 0)
    {
        nSprite = insertsprite(nSector, 108);
        assert(nSprite >= 0 && nSprite < kMaxSprites);
		pSprite = &sprite[nSprite];

        pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z;
    }
    else
    {
        nAngle = pSprite->ang;
        changespritestat(nSprite, 108);
    }

    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->clipdist = 30;
    pSprite->ang = nAngle;
    pSprite->xrepeat = 50;
    pSprite->yrepeat = 50;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->extra = -1;

    if (nAngle >= 0) {
        RatList[nRat].nAction = 2;
    }
    else {
        RatList[nRat].nAction = 4;
    }

    RatList[nRat].nFrame = 0;
    RatList[nRat].nSprite = nSprite;
    RatList[nRat].nTarget = -1;
    RatList[nRat].nCount = RandomSize(5);
    RatList[nRat].nIndex = RandomSize(3);

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nRat, 0x240000);

    RatList[nRat].nRun = runlist_AddRunRec(NewRun, nRat, 0x240000);
}

int FindFood(short nSprite)
{
	auto pSprite = &sprite[nSprite];

    short nSector = pSprite->sectnum;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;

    int z2 = (z + sector[nSector].ceilingz) / 2;

    if (nChunkTotal)
    {
        auto pActor2 = nChunkSprite[RandomSize(7) % nChunkTotal];
		if (pActor2 != nullptr)
		{
			auto pSprite2 = &pActor2->s();
            if (cansee(x, y, z2, nSector, pSprite2->x, pSprite2->y, pSprite2->z, pSprite2->sectnum)) {
                return pActor2->GetSpriteIndex();
            }
        }
    }

    if (!nBodyTotal) {
        return -1;
    }

    auto pActor2 = nBodySprite[RandomSize(7) % nBodyTotal];
    if (pActor2 != nullptr)
    {
		auto pSprite2 = &pActor2->s();
        if (nPlayerPic == pSprite2->picnum)
        {
            if (cansee(x, y, z, nSector, pSprite2->x, pSprite2->y, pSprite2->z, pSprite2->sectnum)) {
                return pActor2->GetSpriteIndex();
            }
        }
    }

    return -1;
}

void AIRat::RadialDamage(RunListEvent* ev)
{
    short nRat = RunData[ev->nRun].nObjIndex;
    short nSprite = RatList[nRat].nSprite;
    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    Damage(ev);
}

void AIRat::Damage(RunListEvent* ev)
{
    short nRat = RunData[ev->nRun].nObjIndex;
    short nSprite = RatList[nRat].nSprite;
    auto pSprite = &sprite[nSprite];

    if (ev->nDamage)
    {
        pSprite->cstat = 0;
        pSprite->xvel = 0;
        pSprite->yvel = 0;
        RatList[nRat].nAction = 3;
        RatList[nRat].nFrame = 0;
    }
    return;
}

void AIRat::Draw(RunListEvent* ev)
{
    short nRat = RunData[ev->nRun].nObjIndex;
    short nAction = RatList[nRat].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRat] + RatSeq[nAction].a, RatList[nRat].nFrame, RatSeq[nAction].b);
}


void AIRat::Tick(RunListEvent* ev)
{
    short nRat = RunData[ev->nRun].nObjIndex;
    short nSprite = RatList[nRat].nSprite;
    short nAction = RatList[nRat].nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    int nSeq = SeqOffsets[kSeqRat] + RatSeq[nAction].a;
    pSprite->picnum = seq_GetSeqPicnum2(nSeq, RatList[nRat].nFrame);

    seq_MoveSequence(nSprite, nSeq, RatList[nRat].nFrame);

    RatList[nRat].nFrame++;
    if (RatList[nRat].nFrame >= SeqSize[nSeq])
    {
        bVal = true;
        RatList[nRat].nFrame = 0;
    }

    short nTarget = RatList[nRat].nTarget;

    Gravity(nSprite);

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        RatList[nRat].nCount--;
        if (RatList[nRat].nCount > 0) {
            return;
        }

        int xVal = abs(pSprite->x - sprite[nTarget].x);
        int yVal = abs(pSprite->y - sprite[nTarget].y);

        if (xVal > 50 || yVal > 50)
        {
            RatList[nRat].nAction = 2;
            RatList[nRat].nFrame = 0;
            RatList[nRat].nTarget = -1;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
            return;
        }

        RatList[nRat].nFrame ^= 1;
        RatList[nRat].nCount = RandomSize(5) + 4;
        RatList[nRat].nIndex--;

        if (RatList[nRat].nIndex <= 0)
        {
            short nFoodSprite = FindFood(nSprite);
            if (nFoodSprite == -1) {
                return;
            }

            RatList[nRat].nTarget = nFoodSprite;

            PlotCourseToSprite(nSprite, nFoodSprite);
            SetRatVel(nSprite);

            RatList[nRat].nAction = 1;
            RatList[nRat].nIndex = 900;
            RatList[nRat].nFrame = 0;
        }

        return;
    }
    case 1:
    {
        RatList[nRat].nIndex--;

        if (RatList[nRat].nIndex <= 0)
        {
            RatList[nRat].nAction = 2;
            RatList[nRat].nFrame = 0;
            RatList[nRat].nTarget = -1;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }

        MoveCreature(nSprite);

        int xVal = abs(pSprite->x - sprite[nTarget].x);
        int yVal = abs(pSprite->y - sprite[nTarget].y);

        if (xVal >= 50 || yVal >= 50)
        {
            RatList[nRat].nCount--;
            if (RatList[nRat].nCount < 0)
            {
                PlotCourseToSprite(nSprite, nTarget);
                SetRatVel(nSprite);

                RatList[nRat].nCount = 32;
            }

            return;
        }

        RatList[nRat].nAction = 0;
        RatList[nRat].nFrame = 0;
        RatList[nRat].nIndex = RandomSize(3);

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        return;
    }
    case 2:
    {
        if (pSprite->xvel || pSprite->yvel || pSprite->zvel) {
            MoveCreature(nSprite);
        }

        RatList[nRat].nCount--;
        if (RatList[nRat].nCount <= 0)
        {
            RatList[nRat].nTarget = FindFood(nSprite);

            if (RatList[nRat].nTarget <= -1)
            {
                RatList[nRat].nCount = RandomSize(6);
                if (pSprite->xvel || pSprite->yvel)
                {
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }

                pSprite->ang = RandomSize(11);
                SetRatVel(nSprite);
                return;
            }
            else
            {
                PlotCourseToSprite(nSprite, RatList[nRat].nTarget);
                SetRatVel(nSprite);
                RatList[nRat].nAction = 1;
                RatList[nRat].nIndex = 900;
                RatList[nRat].nFrame = 0;
                return;
            }
        }

        return;
    }
    case 3:
    {
        if (bVal)
        {
            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
            runlist_SubRunRec(RatList[nRat].nRun);

            pSprite->cstat = 0x8000;
            mydeletesprite(nSprite);
        }
        return;
    }
    }
}


void FuncRat(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRat ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
