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

static actionSeq SetSeq[] = {
    {0, 0},
    {77, 1},
    {78, 1},
    {0, 0},
    {9, 0},
    {63, 0},
    {45, 0},
    {18, 0},
    {27, 0},
    {36, 0},
    {72, 1},
    {74, 1}
};

struct Set
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nCount;
    short nIndex;
    short nIndex2;
    short nRun;
    short nChannel;
};

TArray<Set> SetList;


FSerializer& Serialize(FSerializer& arc, const char* keyname, Set& w, Set* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("count", w.nCount)
            ("index", w.nIndex)
            ("index2", w.nIndex2)
            ("run", w.nRun)
            ("channel", w.nChannel)
            .EndObject();
    }
    return arc;
}

void SerializeSet(FSerializer& arc)
{
    arc("set", SetList);
}


void InitSets()
{
    SetList.Clear();
}

void BuildSet(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    auto nSet = SetList.Reserve(1);
    auto pActor = &SetList[nSet];
    auto pSprite = &sprite[nSprite];
    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 120);
        pSprite = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 120);
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
    pSprite->shade = -12;
    pSprite->clipdist = 110;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->xrepeat = 87;
    pSprite->yrepeat = 96;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->picnum = 1;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

    //	GrabTimeSlot(3);

    pActor->nAction = 1;
    pActor->nHealth = 8000;
    pActor->nSprite = nSprite;
    pActor->nFrame = 0;
    pActor->nTarget = -1;
    pActor->nCount = 90;
    pActor->nIndex = 0;
    pActor->nIndex2 = 0;

    pActor->nChannel = nChannel;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nSet, 0x190000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, nSet, 0x190000);

    nCreaturesTotal++;
}

void BuildSoul(int nSet)
{
    auto pActor = &SetList[nSet];
    int nSetSprite = pActor->nSprite;
    auto pSetSprite = &sprite[nSetSprite];
    int nSprite = insertsprite(pSetSprite->sectnum, 0);
    auto pSprite = &sprite[nSprite];

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->cstat = 0x8000;
    pSprite->shade = -127;
    pSprite->xrepeat = 1;
    pSprite->yrepeat = 1;
    pSprite->pal = 0;
    pSprite->clipdist = 5;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqSet, 75, 0);
    pSprite->ang = RandomSize(11);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = (-256) - RandomSize(10);
    pSprite->x = pSetSprite->x;
    pSprite->y = pSetSprite->y;

    short nSector = pSprite->sectnum;
    pSprite->z = (RandomSize(8) << 8) + 8192 + sector[nSector].ceilingz - GetSpriteHeight(nSprite);

    pSprite->hitag = nSet;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = 0;

    //	GrabTimeSlot(3);

    pSprite->owner = runlist_AddRunRec(NewRun, nSprite, 0x230000);
}

void AISoul::Tick(RunListEvent* ev)
{
    short nSprite = RunData[ev->nRun].nObjIndex;
    auto pSprite = &sprite[nSprite];

    seq_MoveSequence(nSprite, SeqOffsets[kSeqSet] + 75, 0);

    if (pSprite->xrepeat < 32)
    {
        pSprite->xrepeat++;
        pSprite->yrepeat++;
    }

    pSprite->extra += (nSprite & 0x0F) + 5;
    pSprite->extra &= kAngleMask;

    int nVel = bcos(pSprite->extra, -7);

    if (movesprite(nSprite, bcos(pSprite->ang) * nVel, bsin(pSprite->ang) * nVel, pSprite->zvel, 5120, 0, CLIPMASK0) & 0x10000)
    {
        int nSet = pSprite->hitag;
        auto pActor = &SetList[nSet];
        int nSetSprite = pActor->nSprite;
        auto pSetSprite = &sprite[nSetSprite];

        pSprite->cstat = 0;
        pSprite->yrepeat = 1;
        pSprite->xrepeat = 1;
        pSprite->x = pSetSprite->x;
        pSprite->y = pSetSprite->y;
        pSprite->z = pSetSprite->z - (GetSpriteHeight(nSetSprite) >> 1);
        mychangespritesect(nSprite, pSetSprite->sectnum);
        return;
    }
}



void FuncSoul(int nObject, int nMessage, int nDamage, int nRun)
{
    AISoul ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void AISet::RadialDamage(RunListEvent* ev)
{
    short nSet = RunData[ev->nRun].nObjIndex;
    assert(nSet >= 0 && nSet < (int)SetList.Size());
    auto pActor = &SetList[nSet];

    short nSprite = pActor->nSprite;
    short nAction = pActor->nAction;

    if (nAction == 5)
    {
        ev->nDamage = runlist_CheckRadialDamage(nSprite);
        // fall through to case 0x80000
    }
    Damage(ev);
}

void AISet::Damage(RunListEvent* ev)
{
    short nSet = RunData[ev->nRun].nObjIndex;
    assert(nSet >= 0 && nSet < (int)SetList.Size());
    auto pActor = &SetList[nSet];

    short nSprite = pActor->nSprite;
    short nAction = pActor->nAction;
    auto pSprite = &sprite[nSprite];

    if (ev->nDamage && pActor->nHealth > 0)
    {
        if (nAction != 1)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);
        }

        if (pActor->nHealth <= 0)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->cstat &= 0xFEFE;

            pActor->nHealth = 0;

            nCreaturesKilled++;

            if (nAction < 10)
            {
                pActor->nFrame = 0;
                pActor->nAction = 10;
            }
        }
        else if (nAction == 1)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
        }
    }
}

void AISet::Draw(RunListEvent* ev)
{
    short nSet = RunData[ev->nRun].nObjIndex;
    assert(nSet >= 0 && nSet < (int)SetList.Size());
    auto pActor = &SetList[nSet];

    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqSet] + SetSeq[nAction].a, pActor->nFrame, SetSeq[nAction].b);
    return;
}

void AISet::Tick(RunListEvent* ev)
{
    short nSet = RunData[ev->nRun].nObjIndex;
    assert(nSet >= 0 && nSet < (int)SetList.Size());
    auto pActor = &SetList[nSet];

    short nSprite = pActor->nSprite;
    short nAction = pActor->nAction;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    Gravity(nSprite);

    short nSeq = SeqOffsets[kSeqSet] + SetSeq[pActor->nAction].a;
    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(nSprite, nSeq, pActor->nFrame);

    if (nAction == 3)
    {
        if (pActor->nIndex2) {
            pActor->nFrame++;
        }
    }

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    short nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    short nTarget = pActor->nTarget;

    if (nTarget > -1 && nAction < 10)
    {
        if (!(sprite[nTarget].cstat & 0x101))
        {
            pActor->nTarget = -1;
            pActor->nAction = 0;
            pActor->nFrame = 0;
            nTarget = -1;
        }
    }

    int nMov = MoveCreature(nSprite);

    pushmove_old(&pSprite->x, &pSprite->y, &pSprite->z, &pSprite->sectnum, pSprite->clipdist << 2, 5120, -5120, CLIPMASK0);

    if (pSprite->zvel > 4000)
    {
        if (nMov & 0x20000)
        {
            SetQuake(nSprite, 100);
        }
    }

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if ((nSet & 0x1F) == (totalmoves & 0x1F))
        {
            if (nTarget < 0)
            {
                nTarget = FindPlayer(nSprite, 1000);
            }

            if (nTarget >= 0)
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;
                pActor->nTarget = nTarget;

                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
            }
        }

        return;
    }

    case 1:
    {
        if (FindPlayer(nSprite, 1000) >= 0)
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
            }
        }

        return;
    }

    case 2:
    {
        if (bVal)
        {
            pActor->nAction = 7;
            pActor->nIndex = 0;
            pActor->nFrame = 0;

            pSprite->xvel = 0;
            pSprite->yvel = 0;

            pActor->nTarget = FindPlayer(nSprite, 1000);
        }
        return;
    }

    case 3:
    {
        if (nTarget != -1)
        {
            if ((nFlag & 0x10) && (nMov & 0x20000))
            {
                SetQuake(nSprite, 100);
            }

            int nCourse = PlotCourseToSprite(nSprite, nTarget);

            if ((nSet & 0x1F) == (totalmoves & 0x1F))
            {
                int nRand = RandomSize(3);

                switch (nRand)
                {
                case 0:
                case 2:
                {
                    pActor->nIndex = 0;
                    pActor->nAction = 7;
                    pActor->nFrame = 0;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }
                case 1:
                {
                    PlotCourseToSprite(nSprite, nTarget);

                    pActor->nAction = 6;
                    pActor->nFrame = 0;
                    pActor->nRun = 5;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }
                default:
                {
                    if (nCourse <= 100)
                    {
                        pActor->nIndex2 = 0;
                    }
                    else
                    {
                        pActor->nIndex2 = 1;
                    }
                    break;
                }
                }
            }

            // loc_338E2
            int nAngle = pSprite->ang & 0xFFF8;
            pSprite->xvel = bcos(nAngle, -1);
            pSprite->yvel = bsin(nAngle, -1);

            if (pActor->nIndex2)
            {
                pSprite->xvel *= 2;
                pSprite->yvel *= 2;
            }

            if ((nMov & 0xC000) == 0x8000)
            {
                short nWall = nMov & 0x3FFF;
                short nSector = wall[nWall].nextsector;

                if (nSector >= 0)
                {
                    if ((pSprite->z - sector[nSector].floorz) < 55000)
                    {
                        if (pSprite->z > sector[nSector].ceilingz)
                        {
                            pActor->nIndex = 1;
                            pActor->nAction = 7;
                            pActor->nFrame = 0;
                            pSprite->xvel = 0;
                            pSprite->yvel = 0;
                            return;
                        }
                    }
                }

                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
                break;
            }
            else if ((nMov & 0xC000) == 0xC000)
            {
                if (nTarget == (nMov & 0x3FFF))
                {
                    int nAng = getangle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);
                    if (AngleDiff(pSprite->ang, nAng) < 64)
                    {
                        pActor->nAction = 4;
                        pActor->nFrame = 0;
                    }
                    break;
                }
                else
                {
                    pActor->nIndex = 1;
                    pActor->nAction = 7;
                    pActor->nFrame = 0;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }
            }

            break;
        }
        else
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            return;
        }
    }

    case 4:
    {
        if (nTarget == -1)
        {
            pActor->nAction = 0;
            pActor->nCount = 50;
        }
        else
        {
            if (PlotCourseToSprite(nSprite, nTarget) >= 768)
            {
                pActor->nAction = 3;
            }
            else if (nFlag & 0x80)
            {
                runlist_DamageEnemy(nTarget, nSprite, 5);
            }
        }

        break;
    }

    case 5:
    {
        if (bVal)
        {
            pActor->nAction = 0;
            pActor->nCount = 15;
        }
        return;
    }

    case 6:
    {
        if (nFlag & 0x80)
        {
            // low 16 bits of returned var contains the sprite index, the high 16 the bullet number
            int nBullet = BuildBullet(nSprite, 11, 0, 0, -1, pSprite->ang, nTarget + 10000, 1);
            SetBulletEnemy(nBullet >> 16, nTarget); // isolate the bullet number (shift off the sprite index)

            pActor->nRun--;
            if (pActor->nRun <= 0 || !RandomBit())
            {
                pActor->nAction = 0;
                pActor->nFrame = 0;
            }
        }
        return;
    }

    case 7:
    {
        if (bVal)
        {
            if (pActor->nIndex)
            {
                pSprite->zvel = -10000;
            }
            else
            {
                pSprite->zvel = -(PlotCourseToSprite(nSprite, nTarget));
            }

            pActor->nAction = 8;
            pActor->nFrame = 0;

            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
        }
        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nFrame = SeqSize[nSeq] - 1;
        }

        if (nMov & 0x20000)
        {
            SetQuake(nSprite, 200);
            pActor->nAction = 9;
            pActor->nFrame = 0;
        }
        return;
    }

    case 9:
    {
        pSprite->xvel >>= 1;
        pSprite->yvel >>= 1;

        if (bVal)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;

            PlotCourseToSprite(nSprite, nTarget);

            pActor->nAction = 6;
            pActor->nFrame = 0;
            pActor->nRun = 5;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
        return;
    }

    case 10:
    {
        if (nFlag & 0x80)
        {
            pSprite->z -= GetSpriteHeight(nSprite);
            BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqSet, 76, 0));
            pSprite->z += GetSpriteHeight(nSprite);
        }

        if (bVal)
        {
            pActor->nAction = 11;
            pActor->nFrame = 0;

            runlist_ChangeChannel(pActor->nChannel, 1);

            for (int i = 0; i < 20; i++)
            {
                BuildSoul(nSet);
            }
        }
        return;
    }

    case 11:
    {
        pSprite->cstat &= 0xFEFE;
        return;
    }
    }

    // loc_33AE3: ?
    if (nAction)
    {
        if (nTarget != -1)
        {
            if (!(sprite[nTarget].cstat & 0x101))
            {
                pActor->nAction = 0;
                pActor->nFrame = 0;
                pActor->nCount = 100;
                pActor->nTarget = -1;
                pSprite->xvel = 0;
                pSprite->yvel = 0;
            }
        }
    }

    return;
}

void FuncSet(int nObject, int nMessage, int nDamage, int nRun)
{
    AISet ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}
END_PS_NS
