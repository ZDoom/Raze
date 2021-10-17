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
    auto pSprite = &sprite[nSprite];
    short nSector = pSprite->sectnum;

    int nLimbSprite = insertsprite(nSector, 118);
    assert(nLimbSprite >= 0 && nLimbSprite < kMaxSprites);
	auto pLimbSprite = &sprite[nLimbSprite];

    pLimbSprite->x = pSprite->x;
    pLimbSprite->y = pSprite->y;
    pLimbSprite->z = pSprite->z - RandomLong() % ebx;
    pLimbSprite->cstat = 0;
    pLimbSprite->shade = -127;
    pLimbSprite->pal = 1;
    pLimbSprite->xvel = (RandomSize(5) - 16) << 8;
    pLimbSprite->yvel = (RandomSize(5) - 16) << 8;
    pLimbSprite->zvel = 2560 - (RandomSize(5) << 8);
    pLimbSprite->xoffset = 0;
    pLimbSprite->yoffset = 0;
    pLimbSprite->xrepeat = 90;
    pLimbSprite->yrepeat = 90;
    pLimbSprite->picnum = (edx & 3) % 3;
    pLimbSprite->hitag = 0;
    pLimbSprite->lotag = runlist_HeadRun() + 1;
    pLimbSprite->clipdist = 0;

//	GrabTimeSlot(3);

    pLimbSprite->extra = -1;
    pLimbSprite->owner = runlist_AddRunRec(pLimbSprite->lotag - 1, nLimbSprite, 0x160000);
    pLimbSprite->hitag = runlist_AddRunRec(NewRun, nLimbSprite, 0x160000);

    return nLimbSprite;
}

void AILavaDudeLimb::Tick(RunListEvent* ev)
{
    short nSprite = RunData[ev->nRun].nObjIndex;
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    auto pSprite = &sprite[nSprite];

    pSprite->shade += 3;

    int nRet = movesprite(nSprite, pSprite->xvel << 12, pSprite->yvel << 12, pSprite->zvel, 2560, -2560, CLIPMASK1);

    if (nRet || pSprite->shade > 100)
    {
        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;

        runlist_DoSubRunRec(pSprite->owner);
        runlist_FreeRun(pSprite->lotag - 1);
        runlist_SubRunRec(pSprite->hitag);

        mydeletesprite(nSprite);
    }
}

void AILavaDudeLimb::Draw(RunListEvent* ev)
{
    short nSprite = RunData[ev->nRun].nObjIndex;
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    auto pSprite = &sprite[nSprite];
    seq_PlotSequence(ev->nParam, (SeqOffsets[kSeqLavag] + 30) + pSprite->picnum, 0, 1);
}

void  FuncLavaLimb(int nObject, int nMessage, int nDamage, int nRun)
{
    AILavaDudeLimb ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void BuildLava(short nSprite, int x, int y, int, short nSector, short nAngle, int nChannel)
{
    auto nLava = LavaList.Reserve(1);

    auto pSprite = &sprite[nSprite];
    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 118);
        pSprite = &sprite[nSprite];
    }
    else
    {
        nSector = pSprite->sectnum;
        nAngle = pSprite->ang;
        x = pSprite->x;
        y = pSprite->y;

        changespritestat(nSprite, 118);
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = sector[nSector].floorz;
    pSprite->cstat = 0x8000;
    pSprite->xrepeat = 200;
    pSprite->yrepeat = 200;
    pSprite->shade = -12;
    pSprite->pal = 0;
    pSprite->clipdist = 127;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqLavag, LavadudeSeq[3].a, 0);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->ang = nAngle;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;

//	GrabTimeSlot(3);

    pSprite->extra = -1;

    LavaList[nLava].nAction = 0;
    LavaList[nLava].nHealth = 4000;
    LavaList[nLava].nSprite = nSprite;
    LavaList[nLava].nTarget = -1;
    LavaList[nLava].nIndex = nChannel;
    LavaList[nLava].nFrame = 0;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nLava, 0x150000);
    LavaList[nLava].nRun = runlist_AddRunRec(NewRun, nLava, 0x150000);

    nCreaturesTotal++;
}

void AILavaDude::Draw(RunListEvent* ev)
{
    unsigned nLava = RunData[ev->nRun].nObjIndex;
    assert(nLava < LavaList.Size());

    short nAction = LavaList[nLava].nAction;
    short nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    seq_PlotSequence(ev->nParam, nSeq, LavaList[nLava].nFrame, LavadudeSeq[nAction].b);
    ev->pTSprite->owner = -1;
    return;
}

void AILavaDude::Damage(RunListEvent* ev)
{
    unsigned nLava = RunData[ev->nRun].nObjIndex;
    assert(nLava < LavaList.Size());

    short nAction = LavaList[nLava].nAction;
    short nSprite = LavaList[nLava].nSprite;
    auto pSprite = &sprite[nSprite];

    if (!ev->nDamage) 
    {
        return;
    }

    LavaList[nLava].nHealth -= dmgAdjust(ev->nDamage, 3);

    if (LavaList[nLava].nHealth <= 0)
    {
        LavaList[nLava].nHealth = 0;
        LavaList[nLava].nAction = 5;
        LavaList[nLava].nFrame = 0;

        nCreaturesKilled++;

        pSprite->cstat &= 0xFEFE;
    }
    else
    {
        short nTarget = ev->nParam;

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
                LavaList[nLava].nFrame = 0;
                pSprite->cstat = 0;
            }
        }

        BuildLavaLimb(nSprite, totalmoves, 64000);
    }
}

void AILavaDude::Tick(RunListEvent* ev)
{
    unsigned nLava = RunData[ev->nRun].nObjIndex;
    assert(nLava < LavaList.Size());

    short nAction = LavaList[nLava].nAction;
    short nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    short nSprite = LavaList[nLava].nSprite;
    auto pSprite = &sprite[nSprite];

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, LavaList[nLava].nFrame);
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

            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);

            if (nTarget >= 0 && !RandomSize(1))
            {
                LavaList[nLava].nTarget = nTarget;
                LavaList[nLava].nAction = 2;
                pSprite->cstat = 0x101;
                LavaList[nLava].nFrame = 0;
                break;
            }
        }

        int x = pSprite->x;
        int y = pSprite->y;
        int z = pSprite->z;
        short nSector = pSprite->sectnum;

        int nVal = movesprite(nSprite, pSprite->xvel << 8, pSprite->yvel << 8, 0, 0, 0, CLIPMASK0);

        if (nSector != pSprite->sectnum)
        {
            changespritesect(nSprite, nSector);
            pSprite->x = x;
            pSprite->y = y;
            pSprite->z = z;

            pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
            break;
        }

        if (!nVal) {
            break;
        }

        if ((nVal & 0xC000) == 0x8000)
        {
            pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
            break;
        }
        else if ((nVal & 0xC000) == 0xC000)
        {
            if ((nVal & 0x3FFF) == nTarget)
            {
                int nAng = getangle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);
                if (AngleDiff(pSprite->ang, nAng) < 64)
                {
                    LavaList[nLava].nAction = 2;
                    LavaList[nLava].nFrame = 0;
                    pSprite->cstat = 0x101;
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

            pSprite->cstat |= 0x101;
        }

        break;
    }

    case 3:
    {
        if ((nFlag & 0x80) && nTarget > -1)
        {
            int nHeight = GetSpriteHeight(nSprite);
            GetUpAngle(nSprite, -64000, nTarget, (-(nHeight >> 1)));

            BuildBullet(nSprite, 10, bcos(pSprite->ang, 8), bsin(pSprite->ang, 8), -1, pSprite->ang, nTarget + 10000, 1);
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
            pSprite->cstat &= 0xFEFE;
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
                } while (ecx < 20);
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
            } while (ecx < 30);

            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
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
            pSprite->cstat = 0x8000;
        }
        break;
    }
    }

    // loc_31521:
    pSprite->pal = 1;
}


void  FuncLava(int nObject, int nMessage, int nDamage, int nRun)
{
    AILavaDude ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}
END_PS_NS
