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
#include "sequence.h"
#include "sound.h"
#include "exhumed.h"
#include <assert.h>
#include "engine.h"

BEGIN_PS_NS

struct Mummy
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nIndex;
    short nCount;
    short nRun;
};

TArray<Mummy> MummyList;

static actionSeq MummySeq[] = {
    {8, 0},
    {0, 0},
    {16, 0},
    {24, 0},
    {32, 1},
    {40, 1},
    {48, 1},
    {50, 0}
};


FSerializer& Serialize(FSerializer& arc, const char* keyname, Mummy& w, Mummy* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("index", w.nIndex)
            ("count", w.nCount)
            ("run", w.nRun)
            .EndObject();
    }
    return arc;
}

void SerializeMummy(FSerializer& arc)
{
    arc("mummy", MummyList);
}


void InitMummy()
{
    MummyList.Clear();
}

void BuildMummy(int nSprite, int x, int y, int z, int nSector, int nAngle)
{
    auto nMummy = MummyList.Reserve(1);
    auto pActor = &MummyList[nMummy];
	auto pSprite = &sprite[nSprite];

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 102);
		pSprite = &sprite[nSprite];
    }
    else
    {
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->z;
        nAngle = pSprite->ang;

        changespritestat(nSprite, 102);
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->clipdist = 32;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->xrepeat = 42;
    pSprite->yrepeat = 42;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->picnum = 1;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 640;
    pActor->nFrame = 0;
    pActor->nSprite = nSprite;
    pActor->nTarget = -1;
    pActor->nIndex = nMummy;
    pActor->nCount = 0;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nMummy, 0xE0000);

    pActor->nRun = runlist_AddRunRec(NewRun, nMummy, 0xE0000);

    nCreaturesTotal++;
}

void CheckMummyRevive(short nMummy)
{
    auto pActor = &MummyList[nMummy];
    short nSprite = pActor->nSprite;
	auto pSprite = &sprite[nSprite];

    for (unsigned i = 0; i < MummyList.Size(); i++)
    {
        if ((int)i != nMummy)
        {
            short nSprite2 = MummyList[i].nSprite;
            if (sprite[nSprite2].statnum != 102) {
                continue;
            }

            if (MummyList[i].nAction != 5) {
                continue;
            }

            int x = abs(sprite[nSprite2].x - pSprite->x) >> 8;
            int y = abs(sprite[nSprite2].y - pSprite->y) >> 8;

            if (x <= 20 && y <= 20)
            {
                if (cansee(pSprite->x, pSprite->y, pSprite->z - 8192, pSprite->sectnum,
                          sprite[nSprite2].x, sprite[nSprite2].y, sprite[nSprite2].z - 8192, sprite[nSprite2].sectnum))
                {
                    sprite[nSprite2].cstat = 0;
                    MummyList[i].nAction = 6;
                    MummyList[i].nFrame = 0;
                }
            }
        }
    }
}

void AIMummy::Tick(RunListEvent* ev)
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    auto pActor = &MummyList[nMummy];

    short nTarget = UpdateEnemy(&pActor->nTarget);

    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pActor->nAction;

    Gravity(nSprite);

    int nSeq = SeqOffsets[kSeqMummy] + MummySeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    short nFrame = SeqBase[nSeq] + pActor->nFrame;
    short nFrameFlag = FrameFlag[nFrame];

    seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

    bool bVal = false;

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;

        bVal = true;
    }

    if (nTarget != -1 && nAction < 4)
    {
        if ((!sprite[nTarget].cstat) && nAction)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }

    int nMov = MoveCreatureWithCaution(nSprite);

    if (nAction > 7)
        return;

    switch (nAction)
    {
    case 0:
    {
        if ((pActor->nIndex & 0x1F) == (totalmoves & 0x1F))
        {
            pSprite->cstat = 0x101;

            if (nTarget < 0)
            {
                int nTarget = FindPlayer(nSprite, 100);
                if (nTarget >= 0)
                {
                    D3PlayFX(StaticSound[kSound7], nSprite);
                    pActor->nFrame = 0;
                    pActor->nTarget = nTarget;
                    pActor->nAction = 1;
                    pActor->nCount = 90;

                    pSprite->xvel = bcos(pSprite->ang, -2);
                    pSprite->yvel = bsin(pSprite->ang, -2);
                }
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

        if ((pActor->nIndex & 0x1F) == (totalmoves & 0x1F))
        {
            pSprite->cstat = 0x101;

            PlotCourseToSprite(nSprite, nTarget);

            if (pActor->nAction == 1)
            {
                if (RandomBit())
                {
                    if (cansee(pSprite->x, pSprite->y, pSprite->z - GetSpriteHeight(nSprite), pSprite->sectnum,
                        sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
                    {
                        pActor->nAction = 3;
                        pActor->nFrame = 0;

                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                        return;
                    }
                }
            }
        }

        // loc_2B5A8
        if (!pActor->nFrame)
        {
            pSprite->xvel = bcos(pSprite->ang, -1);
            pSprite->yvel = bsin(pSprite->ang, -1);
        }

        if (pSprite->xvel || pSprite->yvel)
        {
            if (pSprite->xvel > 0)
            {
                pSprite->xvel -= 1024;
                if (pSprite->xvel < 0) {
                    pSprite->xvel = 0;
                }
            }
            else if (pSprite->xvel < 0)
            {
                pSprite->xvel += 1024;
                if (pSprite->xvel > 0) {
                    pSprite->xvel = 0;
                }
            }

            if (pSprite->yvel > 0)
            {
                pSprite->yvel -= 1024;
                if (pSprite->yvel < 0) {
                    pSprite->yvel = 0;
                }
            }
            else if (pSprite->yvel < 0)
            {
                pSprite->yvel += 1024;
                if (pSprite->yvel > 0) {
                    pSprite->yvel = 0;
                }
            }
        }

        if (nMov)
        {
            switch (nMov & 0xC000)
            {
            case 0x8000:
            {
                pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -2);
                pSprite->yvel = bsin(pSprite->ang, -2);
                return;
            }

            case 0xC000:
            {
                if ((nMov & 0x3FFF) == nTarget)
                {
                    int nAngle = getangle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);
                    if (AngleDiff(pSprite->ang, nAngle) < 64)
                    {
                        pActor->nAction = 2;
                        pActor->nFrame = 0;

                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                    }
                }
                return;
            }
            }
        }

        break;
    }

    case 2:
    {
        if (nTarget == -1)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 1024)
            {
                pActor->nAction = 1;
                pActor->nFrame = 0;
            }
            else if (nFrameFlag & 0x80)
            {
                runlist_DamageEnemy(nTarget, nSprite, 5);
            }
        }
        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 0;
            pActor->nCount = 100;
            pActor->nTarget = -1;
            return;
        }
        else if (nFrameFlag & 0x80)
        {
            SetQuake(nSprite, 100);

            // low 16 bits of returned var contains the sprite index, the high 16 the bullet number
            int nBullet = BuildBullet(nSprite, 9, 0, 0, -15360, pSprite->ang, nTarget + 10000, 1);
            CheckMummyRevive(nMummy);

            if (nBullet > -1)
            {
                if (!RandomSize(3))
                {
                    // FIXME CHECKME - nBullet & 0xFFFF can be -1. Original code doesn't handle this??

                    SetBulletEnemy(FixedToInt(nBullet), nTarget); // isolate the bullet number (shift off the sprite index)
                    sprite[nBullet & 0xFFFF].pal = 5;
                }
            }
        }
        return;
    }

    case 4:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 5;
        }
        return;
    }

    case 5:
    {
        pActor->nFrame = 0;
        return;
    }

    case 6:
    {
        if (bVal)
        {
            pSprite->cstat = 0x101;

            pActor->nAction = 0;
            pActor->nHealth = 300;
            pActor->nTarget = -1;

            nCreaturesTotal++;
        }
        return;
    }

    case 7:
    {
        if (nMov & 0x20000)
        {
            pSprite->xvel >>= 1;
            pSprite->yvel >>= 1;
        }

        if (bVal)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->cstat = 0x101;

            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->nTarget = -1;
        }

        return;
    }
    }
}

void AIMummy::Draw(RunListEvent* ev)
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    auto pActor = &MummyList[nMummy];
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqMummy] + MummySeq[nAction].a, pActor->nFrame, MummySeq[nAction].b);
    return;
}

void AIMummy::RadialDamage(RunListEvent* ev)
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    auto pActor = &MummyList[nMummy];
    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];

    if (pActor->nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    Damage(ev);
}

void AIMummy::Damage(RunListEvent* ev) 
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    auto pActor = &MummyList[nMummy];

    short nSprite = pActor->nSprite;
    auto pSprite = &sprite[nSprite];

    if (ev->nDamage <= 0)
        return;

    if (pActor->nHealth <= 0) {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pSprite->cstat &= 0xFEFE;
        nCreaturesKilled++;

        DropMagic(nSprite);

        pActor->nFrame = 0;
        pActor->nAction = 4;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->z = sector[pSprite->sectnum].floorz;
    }
    else
    {
        if (!RandomSize(2))
        {
            pActor->nAction = 7;
            pActor->nFrame = 0;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }

    return;
}

void FuncMummy(int nObject, int nMessage, int nDamage, int nRun)
{
    AIMummy ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
