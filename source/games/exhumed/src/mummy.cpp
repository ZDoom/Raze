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

    MummyList[nMummy].nAction = 0;
    MummyList[nMummy].nHealth = 640;
    MummyList[nMummy].nFrame = 0;
    MummyList[nMummy].nSprite = nSprite;
    MummyList[nMummy].nTarget = -1;
    MummyList[nMummy].nIndex = nMummy;
    MummyList[nMummy].nCount = 0;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nMummy, 0xE0000);

    MummyList[nMummy].nRun = runlist_AddRunRec(NewRun, nMummy, 0xE0000);

    nCreaturesTotal++;
}

void CheckMummyRevive(short nMummy)
{
    short nSprite = MummyList[nMummy].nSprite;
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

    short nTarget = UpdateEnemy(&MummyList[nMummy].nTarget);

    short nSprite = MummyList[nMummy].nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = MummyList[nMummy].nAction;

    Gravity(nSprite);

    int nSeq = SeqOffsets[kSeqMummy] + MummySeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, MummyList[nMummy].nFrame);

    short nFrame = SeqBase[nSeq] + MummyList[nMummy].nFrame;
    short nFrameFlag = FrameFlag[nFrame];

    seq_MoveSequence(nSprite, nSeq, MummyList[nMummy].nFrame);

    bool bVal = false;

    MummyList[nMummy].nFrame++;
    if (MummyList[nMummy].nFrame >= SeqSize[nSeq])
    {
        MummyList[nMummy].nFrame = 0;

        bVal = true;
    }

    if (nTarget != -1 && nAction < 4)
    {
        if ((!sprite[nTarget].cstat) && nAction)
        {
            MummyList[nMummy].nAction = 0;
            MummyList[nMummy].nFrame = 0;
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
        if ((MummyList[nMummy].nIndex & 0x1F) == (totalmoves & 0x1F))
        {
            pSprite->cstat = 0x101;

            if (nTarget < 0)
            {
                int nTarget = FindPlayer(nSprite, 100);
                if (nTarget >= 0)
                {
                    D3PlayFX(StaticSound[kSound7], nSprite);
                    MummyList[nMummy].nFrame = 0;
                    MummyList[nMummy].nTarget = nTarget;
                    MummyList[nMummy].nAction = 1;
                    MummyList[nMummy].nCount = 90;

                    pSprite->xvel = bcos(pSprite->ang, -2);
                    pSprite->yvel = bsin(pSprite->ang, -2);
                }
            }
        }
        return;
    }

    case 1:
    {
        if (MummyList[nMummy].nCount > 0)
        {
            MummyList[nMummy].nCount--;
        }

        if ((MummyList[nMummy].nIndex & 0x1F) == (totalmoves & 0x1F))
        {
            pSprite->cstat = 0x101;

            PlotCourseToSprite(nSprite, nTarget);

            if (MummyList[nMummy].nAction == 1)
            {
                if (RandomBit())
                {
                    if (cansee(pSprite->x, pSprite->y, pSprite->z - GetSpriteHeight(nSprite), pSprite->sectnum,
                        sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
                    {
                        MummyList[nMummy].nAction = 3;
                        MummyList[nMummy].nFrame = 0;

                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                        return;
                    }
                }
            }
        }

        // loc_2B5A8
        if (!MummyList[nMummy].nFrame)
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
                        MummyList[nMummy].nAction = 2;
                        MummyList[nMummy].nFrame = 0;

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
            MummyList[nMummy].nAction = 0;
            MummyList[nMummy].nFrame = 0;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 1024)
            {
                MummyList[nMummy].nAction = 1;
                MummyList[nMummy].nFrame = 0;
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
            MummyList[nMummy].nFrame = 0;
            MummyList[nMummy].nAction = 0;
            MummyList[nMummy].nCount = 100;
            MummyList[nMummy].nTarget = -1;
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
            MummyList[nMummy].nFrame = 0;
            MummyList[nMummy].nAction = 5;
        }
        return;
    }

    case 5:
    {
        MummyList[nMummy].nFrame = 0;
        return;
    }

    case 6:
    {
        if (bVal)
        {
            pSprite->cstat = 0x101;

            MummyList[nMummy].nAction = 0;
            MummyList[nMummy].nHealth = 300;
            MummyList[nMummy].nTarget = -1;

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

            MummyList[nMummy].nAction = 0;
            MummyList[nMummy].nFrame = 0;
            MummyList[nMummy].nTarget = -1;
        }

        return;
    }
    }
}

void AIMummy::Draw(RunListEvent* ev)
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    short nAction = MummyList[nMummy].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqMummy] + MummySeq[nAction].a, MummyList[nMummy].nFrame, MummySeq[nAction].b);
    return;
}

void AIMummy::RadialDamage(RunListEvent* ev)
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);
    short nSprite = MummyList[nMummy].nSprite;
    auto pSprite = &sprite[nSprite];

    if (MummyList[nMummy].nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    Damage(ev);
}

void AIMummy::Damage(RunListEvent* ev) 
{
    short nMummy = RunData[ev->nRun].nObjIndex;
    assert(nMummy >= 0 && nMummy < kMaxMummies);

    short nSprite = MummyList[nMummy].nSprite;
    auto pSprite = &sprite[nSprite];

    if (ev->nDamage <= 0)
        return;

    if (MummyList[nMummy].nHealth <= 0) {
        return;
    }

    MummyList[nMummy].nHealth -= dmgAdjust(ev->nDamage);

    if (MummyList[nMummy].nHealth <= 0)
    {
        MummyList[nMummy].nHealth = 0;
        pSprite->cstat &= 0xFEFE;
        nCreaturesKilled++;

        DropMagic(nSprite);

        MummyList[nMummy].nFrame = 0;
        MummyList[nMummy].nAction = 4;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->z = sector[pSprite->sectnum].floorz;
    }
    else
    {
        if (!RandomSize(2))
        {
            MummyList[nMummy].nAction = 7;
            MummyList[nMummy].nFrame = 0;

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
