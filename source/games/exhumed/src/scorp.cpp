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

    scorpion[nScorp].nHealth = 20000;
    scorpion[nScorp].nFrame = 0;
    scorpion[nScorp].nAction = 0;
    scorpion[nScorp].nSprite = nSprite;
    scorpion[nScorp].nTarget = -1;
    scorpion[nScorp].nCount = 0;
    scorpion[nScorp].nIndex2 = 1;

    scorpion[nScorp].nChannel = nChannel;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nScorp, 0x220000);
    scorpion[nScorp].nRun = runlist_AddRunRec(NewRun, nScorp, 0x220000);

    nCreaturesTotal++;
}

void AIScorp::Draw(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    short nAction = scorpion[nScorp].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a, scorpion[nScorp].nFrame, ScorpSeq[nAction].b);
}

void AIScorp::RadialDamage(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    short nSprite = scorpion[nScorp].nSprite;

    ev->nDamage = runlist_CheckRadialDamage(nSprite);
    if (ev->nDamage) Damage(ev);
}


void AIScorp::Damage(RunListEvent* ev)
{
    short nScorp = RunData[ev->nRun].nObjIndex;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());
    short nSprite = scorpion[nScorp].nSprite;

    short nAction = scorpion[nScorp].nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    short nTarget = -1;

    if (scorpion[nScorp].nHealth <= 0) {
        return;
    }

    scorpion[nScorp].nHealth -= dmgAdjust(ev->nDamage);

    if (scorpion[nScorp].nHealth <= 0)
    {
        scorpion[nScorp].nHealth = 0;
        scorpion[nScorp].nAction = 4;
        scorpion[nScorp].nFrame = 0;
        scorpion[nScorp].nCount = 10;

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
                scorpion[nScorp].nTarget = nTarget;
            }
        }

        if (!RandomSize(5))
        {
            scorpion[nScorp].nAction = RandomSize(2) + 4;
            scorpion[nScorp].nFrame = 0;
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
    short nSprite = scorpion[nScorp].nSprite;

    short nAction = scorpion[nScorp].nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    short nTarget = -1;

    if (scorpion[nScorp].nHealth) {
        Gravity(nSprite);
    }

    int nSeq = SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, scorpion[nScorp].nFrame);
    seq_MoveSequence(nSprite, nSeq, scorpion[nScorp].nFrame);

    scorpion[nScorp].nFrame++;

    if (scorpion[nScorp].nFrame >= SeqSize[nSeq])
    {
        scorpion[nScorp].nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + scorpion[nScorp].nFrame];
    nTarget = scorpion[nScorp].nTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if (scorpion[nScorp].nCount > 0)
        {
            scorpion[nScorp].nCount--;
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

                    scorpion[nScorp].nFrame = 0;
                    pSprite->xvel = bcos(pSprite->ang);
                    pSprite->yvel = bsin(pSprite->ang);

                    scorpion[nScorp].nAction = 1;
                    scorpion[nScorp].nTarget = nTarget;
                }
            }
        }

        return;
    }

    case 1:
    {
        scorpion[nScorp].nIndex2--;

        if (scorpion[nScorp].nIndex2 <= 0)
        {
            scorpion[nScorp].nIndex2 = RandomSize(5);
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
                        scorpion[nScorp].nAction = 2;
                        scorpion[nScorp].nFrame = 0;
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
            scorpion[nScorp].nAction = 0;
            scorpion[nScorp].nCount = 5;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 768)
            {
                scorpion[nScorp].nAction = 1;
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
            scorpion[nScorp].nIndex--;
            if (scorpion[nScorp].nIndex <= 0)
            {
                scorpion[nScorp].nAction = 1;

                pSprite->xvel = bcos(pSprite->ang);
                pSprite->yvel = bsin(pSprite->ang);

                scorpion[nScorp].nFrame = 0;
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

        if (scorpion[nScorp].nHealth > 0)
        {
            scorpion[nScorp].nAction = 1;
            scorpion[nScorp].nFrame = 0;
            scorpion[nScorp].nCount = 0;
            return;
        }

        scorpion[nScorp].nCount--;
        if (scorpion[nScorp].nCount <= 0)
        {
            scorpion[nScorp].nAction = 8;
        }
        else
        {
            scorpion[nScorp].nAction = RandomBit() + 6;
        }

        return;
    }

    case 8:
    {
        if (bVal)
        {
            scorpion[nScorp].nAction++; // set to 9
            scorpion[nScorp].nFrame = 0;

            runlist_ChangeChannel(scorpion[nScorp].nChannel, 1);
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
            runlist_SubRunRec(scorpion[nScorp].nRun);
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
    short nSprite = scorpion[nScorp].nSprite;

    short nAction = scorpion[nScorp].nAction;
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
        if (scorpion[nScorp].nCount)
        {
            scorpion[nScorp].nCount--;
        }
        else
        {
            scorpion[nScorp].nCount = 45;

            if (cansee(pSprite->x, pSprite->y, pSprite->z - GetSpriteHeight(nSprite), pSprite->sectnum,
                sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);

                scorpion[nScorp].nIndex = RandomSize(2) + RandomSize(3);

                if (!scorpion[nScorp].nIndex) {
                    scorpion[nScorp].nCount = RandomSize(5);
                }
                else
                {
                    scorpion[nScorp].nAction = 3;
                    scorpion[nScorp].nFrame = 0;
                }
            }
        }
    }

    if (!nAction || nTarget == -1) {
        return;
    }

    if (!(sprite[nTarget].cstat & 0x101))
    {
        scorpion[nScorp].nAction = 0;
        scorpion[nScorp].nFrame = 0;
        scorpion[nScorp].nCount = 30;
        scorpion[nScorp].nTarget = -1;

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
