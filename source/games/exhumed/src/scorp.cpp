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
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

/*
    Selkis Boss AI code
*/

struct Scorpion
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nRun;
    short nCount;
    short nIndex;
    int8_t nIndex2;
    short nChannel;
};

TArray<Scorpion> scorpion;

static actionSeq ScorpSeq[] = {
    {0, 0},
    {8, 0},
    {29, 0},
    {19, 0},
    {45, 1},
    {46, 1},
    {47, 1},
    {48, 1},
    {50, 1},
    {53, 1}
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Scorpion& w, Scorpion* def)
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
            ("index2", w.nIndex2)
            ("chan", w.nChannel)
            .EndObject();
    }
    return arc;
}

void SerializeScorpion(FSerializer& arc)
{
    arc("scorpion", scorpion);
}

void InitScorp()
{
    scorpion.Clear();
}

void BuildScorp(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    auto nScorp = scorpion.Reserve(1);
    auto pActor = &scorpion[nScorp];

    auto pSprite = &sprite[nSprite];

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 122);
        pSprite = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 122);

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
    pSprite->clipdist = 70;
    pSprite->shade = -12;
    pSprite->xrepeat = 80;
    pSprite->yrepeat = 80;
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

    //	GrabTimeSlot(3);

    pActor->nHealth = 20000;
    pActor->nFrame = 0;
    pActor->nAction = 0;
    pActor->nSprite = nSprite;
    pActor->nTarget = -1;
    pActor->nCount = 0;
    pActor->nIndex2 = 1;

    pActor->nChannel = nChannel;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nScorp, 0x220000);
    pActor->nRun = runlist_AddRunRec(NewRun, nScorp, 0x220000);

    nCreaturesTotal++;
}

void AIScorp::Draw(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    auto pActor = &scorpion[nScorp];
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a, pActor->nFrame, ScorpSeq[nAction].b);
}

void AIScorp::RadialDamage(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    auto pActor = &scorpion[nScorp];
    short nSprite = pActor->nSprite;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    if (ev->nDamage) Damage(ev);
}


void AIScorp::Damage(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    auto pActor = &scorpion[nScorp];
    short nSprite = pActor->nSprite;

    short nAction = pActor->nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    short nTarget = -1;

    if (pActor->nHealth <= 0) {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pActor->nAction = 4;
        pActor->nFrame = 0;
        pActor->nCount = 10;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->cstat &= 0xFEFE;

        nCreaturesKilled++;
        return;
    }
    else
    {
        nTarget = ev->nParam;

        if (nTarget >= 0)
        {
            if (pSprite->statnum == 100 || (pSprite->statnum < 199 && !RandomSize(5)))
            {
                pActor->nTarget = nTarget;
            }
        }

        if (!RandomSize(5))
        {
            pActor->nAction = RandomSize(2) + 4;
            pActor->nFrame = 0;
            return;
        }

        if (RandomSize(2)) {
            return;
        }

        D3PlayFX(StaticSound[kSound41], nSprite);
        Effect(ev, nTarget, 0);
    }
}

void AIScorp::Tick(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    auto pActor = &scorpion[nScorp];
    short nSprite = pActor->nSprite;

    short nAction = pActor->nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    short nTarget = -1;

    if (pActor->nHealth) {
        Gravity(nSprite);
    }

    int nSeq = SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    nTarget = pActor->nTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if (pActor->nCount > 0)
        {
            pActor->nCount--;
            return;
        }

        if ((nScorp & 0x1F) == (totalmoves & 0x1F))
        {
            if (nTarget < 0)
            {
                nTarget = FindPlayer(nSprite, 500);

                if (nTarget >= 0)
                {
                    D3PlayFX(StaticSound[kSound41], nSprite);

                    pActor->nFrame = 0;
                    pSprite->xvel = bcos(pSprite->ang);
                    pSprite->yvel = bsin(pSprite->ang);

                    pActor->nAction = 1;
                    pActor->nTarget = nTarget;
                }
            }
        }

        return;
    }

    case 1:
    {
        pActor->nIndex2--;

        if (pActor->nIndex2 <= 0)
        {
            pActor->nIndex2 = RandomSize(5);
            Effect(ev, nTarget, 0);
        }
        else
        {
            int nMov = MoveCreatureWithCaution(nSprite);
            if ((nMov & 0xC000) == 0xC000)
            {
                if (nTarget == (nMov & 0x3FFF))
                {
                    int nAngle = getangle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);
                    if (AngleDiff(pSprite->ang, nAngle) < 64)
                    {
                        pActor->nAction = 2;
                        pActor->nFrame = 0;
                    }
                    Effect(ev, nTarget, 2);
                }
                else
                {
                    Effect(ev, nTarget, 0);
                }
                return;
            }
            else if ((nMov & 0xC000) == 0x8000)
            {
                Effect(ev, nTarget, 0);
            }
            else
            {
                Effect(ev, nTarget, 1);
            }
        }
        return;
    }

    case 2:
    {
        if (nTarget == -1)
        {
            pActor->nAction = 0;
            pActor->nCount = 5;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 768)
            {
                pActor->nAction = 1;
            }
            else if (nFlag & 0x80)
            {
                runlist_DamageEnemy(nTarget, nSprite, 7);
            }
        }
        Effect(ev, nTarget, 2);
        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nIndex--;
            if (pActor->nIndex <= 0)
            {
                pActor->nAction = 1;

                pSprite->xvel = bcos(pSprite->ang);
                pSprite->yvel = bsin(pSprite->ang);

                pActor->nFrame = 0;
                return;
            }
        }

        if (!(nFlag & 0x80)) {
            return;
        }

        int nBulletSprite = BuildBullet(nSprite, 16, 0, 0, -1, pSprite->ang, nTarget + 10000, 1);
        if (nBulletSprite > -1)
        {
            PlotCourseToSprite(nBulletSprite & 0xffff, nTarget);
        }

        return;
    }

    case 4:
    case 5:
    case 6:
    case 7:
    {
        if (!bVal) {
            return;
        }

        if (pActor->nHealth > 0)
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 0;
            return;
        }

        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->nAction = 8;
        }
        else
        {
            pActor->nAction = RandomBit() + 6;
        }

        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nAction++; // set to 9
            pActor->nFrame = 0;

            runlist_ChangeChannel(pActor->nChannel, 1);
            return;
        }

        int nSpiderSprite = BuildSpider(-1, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, pSprite->ang);
        if (nSpiderSprite != -1)
        {
            sprite[nSpiderSprite].ang = RandomSize(11);

            int nVel = RandomSize(5) + 1;

            sprite[nSpiderSprite].xvel = bcos(sprite[nSpiderSprite].ang, -8) * nVel;
            sprite[nSpiderSprite].yvel = bsin(sprite[nSpiderSprite].ang, -8) * nVel;
            sprite[nSpiderSprite].zvel = (-(RandomSize(5) + 3)) << 8;
        }

        return;
    }

    case 9:
    {
        pSprite->cstat &= 0xFEFE;

        if (bVal)
        {
            runlist_SubRunRec(pActor->nRun);
            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);

            mydeletesprite(nSprite);
        }

        return;
    }
    }
}

void AIScorp::Effect(RunListEvent* ev, int nTarget, int mode)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    auto pActor = &scorpion[nScorp];
    short nSprite = pActor->nSprite;

    short nAction = pActor->nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    if (mode == 0)
    {
        PlotCourseToSprite(nSprite, nTarget);
        pSprite->ang += RandomSize(7) - 63;
        pSprite->ang &= kAngleMask;

        pSprite->xvel = bcos(pSprite->ang);
        pSprite->yvel = bsin(pSprite->ang);
    }
    if (mode <= 1)
    {
        if (pActor->nCount)
        {
            pActor->nCount--;
        }
        else
        {
            pActor->nCount = 45;

            if (cansee(pSprite->x, pSprite->y, pSprite->z - GetSpriteHeight(nSprite), pSprite->sectnum,
                sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);

                pActor->nIndex = RandomSize(2) + RandomSize(3);

                if (!pActor->nIndex) {
                    pActor->nCount = RandomSize(5);
                }
                else
                {
                    pActor->nAction = 3;
                    pActor->nFrame = 0;
                }
            }
        }
    }

    if (!nAction || nTarget == -1) {
        return;
    }

    if (!(sprite[nTarget].cstat & 0x101))
    {
        pActor->nAction = 0;
        pActor->nFrame = 0;
        pActor->nCount = 30;
        pActor->nTarget = -1;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
    }
}


void FuncScorp(int nObject, int nMessage, int nDamage, int nRun)
{
    AIScorp ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}
END_PS_NS
