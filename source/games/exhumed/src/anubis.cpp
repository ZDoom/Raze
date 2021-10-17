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
#include "exhumed.h"
#include "aistuff.h"
#include "engine.h"
#include "sequence.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

struct Anubis
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nCount;
};

static TArray<Anubis> AnubisList;
static int nAnubisDrum = 0;

static actionSeq AnubisSeq[] = {
    { 0, 0 },
    { 8, 0 },
    { 16, 0 },
    { 24, 0 },
    { 32, 0 },
    { -1, 0 },
    { 46, 1 },
    { 46, 1 },
    { 47, 1 },
    { 49, 1 },
    { 49, 1 },
    { 40, 1 },
    { 42, 1 },
    { 41, 1 },
    { 43, 1 },
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Anubis& w, Anubis* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("count", w.nCount)
            .EndObject();
    }
    return arc;
}

void SerializeAnubis(FSerializer& arc)
{
    arc("anubis", AnubisList)
        ("anubisdrum", nAnubisDrum);
}

void InitAnubis()
{
    AnubisList.Clear();
    nAnubisDrum = 1;
}

void BuildAnubis(int nSprite, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer)
{
    auto nAnubis = AnubisList.Reserve(1);
    auto ap = &AnubisList[nAnubis];

    spritetype* sp;
    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 101);
        sp = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 101);
        sp = &sprite[nSprite];

        x = sp->x;
        y = sp->y;
        z = sector[sp->sectnum].floorz;
        nAngle = sp->ang;
    }

    assert(nSprite >=0 && nSprite < kMaxSprites);

    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0x101;
    sp->xoffset = 0;
    sp->shade = -12;
    sp->yoffset = 0;
    sp->picnum = 1;
    sp->pal = sector[sp->sectnum].ceilingpal;
    sp->clipdist = 60;
    sp->ang = nAngle;
    sp->xrepeat = 40;
    sp->yrepeat = 40;
    sp->xvel = 0;
    sp->yvel = 0;
    sp->zvel = 0;
    sp->hitag = 0;
    sp->lotag = runlist_HeadRun() + 1;
    sp->extra = -1;

//	GrabTimeSlot(3);

    if (bIsDrummer)
    {
        ap->nAction = nAnubisDrum + 6;
        nAnubisDrum++;

        if (nAnubisDrum >= 5) {
            nAnubisDrum = 0;
        }
    }
    else
    {
        ap->nAction = 0;
    }

    ap->nHealth = 540;
    ap->nFrame  = 0;
    ap->nSprite = nSprite;
    ap->nTarget = -1;
    ap->nCount = 0;

    sp->owner = runlist_AddRunRec(sp->lotag - 1, nAnubis, 0x90000);

    runlist_AddRunRec(NewRun, nAnubis, 0x90000);
    nCreaturesTotal++;
}

void AIAnubis::Tick(RunListEvent* ev)
{
    int nAnubis = RunData[ev->nRun].nObjIndex;
    auto ap = &AnubisList[nAnubis];
    int nSprite = ap->nSprite;
    auto sp = &sprite[nSprite];
    int nAction = ap->nAction;

    bool bVal = false;

    if (nAction < 11) {
        Gravity(nSprite);
    }

    short nSeq = SeqOffsets[kSeqAnubis] + AnubisSeq[nAction].a;

    seq_MoveSequence(nSprite, nSeq, ap->nFrame);

    sp->picnum = seq_GetSeqPicnum2(nSeq, ap->nFrame);

    ap->nFrame++;
    if (ap->nFrame >= SeqSize[nSeq])
    {
        ap->nFrame = 0;
        bVal = true;
    }

    short nTarget = ap->nTarget;
    auto pTarget = nTarget < 0 ? nullptr : &sprite[nTarget];

    short nFrame = SeqBase[nSeq] + ap->nFrame;
    short nFlag = FrameFlag[nFrame];

    int nMov = 0;

    if (nAction > 0 && nAction < 11) {
        nMov = MoveCreatureWithCaution(nSprite);
    }

    switch (nAction)
    {
    case 0:
    {
        if ((nAnubis & 0x1F) == (totalmoves & 0x1F))
        {
            if (nTarget < 0) {
                nTarget = FindPlayer(nSprite, 100);
            }

            if (nTarget >= 0)
            {
                D3PlayFX(StaticSound[kSound8], nSprite);
                ap->nAction = 1;
                ap->nFrame = 0;
                ap->nTarget = nTarget;

                sp->xvel = bcos(sp->ang, -2);
                sp->yvel = bsin(sp->ang, -2);
            }
        }
        return;
    }
    case 1:
    {
        if ((nAnubis & 0x1F) == (totalmoves & 0x1F))
        {
            PlotCourseToSprite(nSprite, nTarget);

            int nAngle = sp->ang & 0xFFF8;
            sp->xvel = bcos(nAngle, -2);
            sp->yvel = bsin(nAngle, -2);
        }

        switch (nMov & 0xC000)
        {
        case 0xC000:
        {
            if ((nMov & 0x3FFF) == nTarget)
            {
                int nAng = getangle(pTarget->x - sp->x, pTarget->y - sp->y);
                int nAngDiff = AngleDiff(sp->ang, nAng);

                if (nAngDiff < 64)
                {
                    ap->nAction = 2;
                    ap->nFrame = 0;
                }
                break;
            }
            // else we fall through to 0x8000
            fallthrough__;
        }
        case 0x8000:
        {
            sp->ang = (sp->ang + 256) & kAngleMask;
            sp->xvel = bcos(sp->ang, -2);
            sp->yvel = bsin(sp->ang, -2);
            break;
        }

        default:
        {
            if (ap->nCount)
            {
                ap->nCount--;
            }
            else
            {
                ap->nCount = 60;

                if (nTarget > -1) // NOTE: nTarget can be -1. this check wasn't in original code. TODO: demo compatiblity?
                {
                    if (cansee(sp->x, sp->y, sp->z - GetSpriteHeight(nSprite), sp->sectnum,
                        pTarget->x, pTarget->y, pTarget->z - GetSpriteHeight(nTarget), pTarget->sectnum))
                    {
                        sp->xvel = 0;
                        sp->yvel = 0;
                        sp->ang = GetMyAngle(pTarget->x - sp->x, pTarget->y - sp->y);

                        ap->nAction = 3;
                        ap->nFrame = 0;
                    }
                }
            }
            break;
        }
        }
        break;
    }
    case 2:
    {
        if (nTarget == -1)
        {
            ap->nAction = 0;
            ap->nCount = 50;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 768)
            {
                ap->nAction = 1;
            }
            else
            {
                if (nFlag & 0x80)
                {
                    runlist_DamageEnemy(nTarget, nSprite, 7);
                }
            }
        }

        break;
    }
    case 3:
    {
        if (bVal)
        {
            ap->nAction = 1;

            sp->xvel = bcos(sp->ang, -2);
            sp->yvel = bsin(sp->ang, -2);
            ap->nFrame = 0;
        }
        else
        {
            // loc_25718:
            if (nFlag & 0x80)
            {
                BuildBullet(nSprite, 8, 0, 0, -1, sp->ang, nTarget + 10000, 1);
            }
        }

        return;
    }
    case 4:
    case 5:
    {
        sp->xvel = 0;
        sp->yvel = 0;

        if (bVal)
        {
            ap->nAction = 1;
            ap->nFrame = 0;
        }
        return;
    }
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    {
        if (bVal)
        {
            ap->nAction = (RandomSize(3) % 5) + 6;
            ap->nFrame = 0;
        }
        return;
    }
    case 11:
    case 12:
    {
        if (bVal)
        {
            ap->nAction = nAction + 2;
            ap->nFrame = 0;

            sp->xvel = 0;
            sp->yvel = 0;
        }
        return;
    }
    case 13:
    case 14:
    {
        sp->cstat &= 0xFEFE;
        return;
    }

    default:
        return;
    }

    // loc_2564C:
    if (nAction && nTarget != -1)
    {
        if (!(pTarget->cstat & 0x101))
        {
            ap->nAction = 0;
            ap->nFrame = 0;
            ap->nCount = 100;
            ap->nTarget = -1;

            sp->xvel = 0;
            sp->yvel = 0;
        }
    }

    return;
}

void AIAnubis::Draw(RunListEvent* ev)
{
    auto ap = &AnubisList[RunData[ev->nRun].nObjIndex];
    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqAnubis] + AnubisSeq[ap->nAction].a, ap->nFrame, AnubisSeq[ap->nAction].b);
}

void AIAnubis::RadialDamage(RunListEvent* ev)
{
    auto ap = &AnubisList[RunData[ev->nRun].nObjIndex];
    if (ap->nAction < 11) 
	{
    	ev->nDamage = runlist_CheckRadialDamage(ap->nSprite);
	    Damage(ev);
	}
}

void AIAnubis::Damage(RunListEvent* ev)
{
    auto ap = &AnubisList[RunData[ev->nRun].nObjIndex];
    int nSprite = ap->nSprite;
    auto sp = &sprite[nSprite];
    int nAction = ap->nAction;
    int nDamage = ev->nDamage;

    if (nDamage)
    {
        if (ap->nHealth <= 0)
            return;

        ap->nHealth -= dmgAdjust(nDamage);

        if (ap->nHealth > 0)
        {
            int nTarget = ev->nParam;

            // loc_258D6:
            if (nTarget < 0) {
                return;
            }
            auto pTarget = &sprite[nTarget];

            if (pTarget->statnum == 100 || pTarget->statnum < 199)
            {
                if (!RandomSize(5)) {
                    ap->nTarget = nTarget;
                }
            }

            if (RandomSize(1))
            {
                if (nAction >= 6 && nAction <= 10)
                {
                    int nDrumSprite = insertsprite(sp->sectnum, kStatAnubisDrum);
                    auto pDrumSprite = &sprite[nDrumSprite];

                    pDrumSprite->x = sp->x;
                    pDrumSprite->y = sp->y;
                    pDrumSprite->z = sector[pDrumSprite->sectnum].floorz;
                    pDrumSprite->xrepeat = 40;
                    pDrumSprite->yrepeat = 40;
                    pDrumSprite->shade = -64;

                    BuildObject(nDrumSprite, 2, 0);
                }

                ap->nAction = 4;
                ap->nFrame = 0;
            }
            else
            {
                // loc_259B5:
                D3PlayFX(StaticSound[kSound39], nSprite);
            }
        }
        else
        {
            // he ded.
            sp->xvel = 0;
            sp->yvel = 0;
            sp->zvel = 0;
            sp->z = sector[sp->sectnum].floorz;
            sp->cstat &= 0xFEFE;

            ap->nHealth = 0;

            nCreaturesKilled++;

            if (nAction < 11)
            {
                DropMagic(nSprite);
                ap->nAction = (ev->nMessage == EMessageType::RadialDamage) + 11;
                ap->nFrame = 0;
            }
        }
    }
}


void FuncAnubis(int nObject, int nMessage, int nDamage, int nRun)
{
    AIAnubis ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
