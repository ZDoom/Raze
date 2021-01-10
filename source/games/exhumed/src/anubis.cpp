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

int BuildAnubis(int nSprite, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer)
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

    sp->owner = runlist_AddRunRec(sp->lotag - 1, nAnubis | 0x90000);

    runlist_AddRunRec(NewRun, nAnubis | 0x90000);
    nCreaturesTotal++;

    return nAnubis | 0x90000;
}

void FuncAnubis(int a, int nDamage, int nRun)
{
    int nAnubis = RunData[nRun].nVal;
    auto ap = &AnubisList[nAnubis];
    assert(nAnubis >= 0 && nAnubis < (int)AnubisList.Size());

    int nSprite = ap->nSprite;
    auto sp = &sprite[nSprite];
    short nAction = ap->nAction;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Anubis\n", nMessage);
            return;
        }

        case 0x20000:
        {
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
                                int nAng = getangle(sprite[nTarget].x - sp->x, sprite[nTarget].y - sp->y);
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
                                        sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
                                    {
                                        sp->xvel = 0;
                                        sp->yvel = 0;
                                        sp->ang = GetMyAngle(sprite[nTarget].x - sp->x, sprite[nTarget].y - sp->y);

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
                if (!(sprite[nTarget].cstat & 0x101))
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

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqAnubis] + AnubisSeq[nAction].a, ap->nFrame, AnubisSeq[nAction].b);
            break;
        }

        case 0xA0000: // fall through to next case
        {
            if (nAction >= 11) {
                return;
            }

            nDamage = runlist_CheckRadialDamage(nSprite);
            fallthrough__;
        }
        case 0x80000:
        {
            if (nDamage)
            {
                if (ap->nHealth <= 0)
                    return;

                ap->nHealth -= nDamage;

                if (ap->nHealth > 0)
                {
                    short nTarget = a & 0xFFFF;

                    // loc_258D6:
                    if (nTarget < 0) {
                        return;
                    }

                    if (sprite[nTarget].statnum == 100 || sprite[nTarget].statnum < 199)
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

                            sprite[nDrumSprite].x = sp->x;
                            sprite[nDrumSprite].y = sp->y;
                            sprite[nDrumSprite].z = sector[sprite[nDrumSprite].sectnum].floorz;
                            sprite[nDrumSprite].xrepeat = 40;
                            sprite[nDrumSprite].yrepeat = 40;
                            sprite[nDrumSprite].shade = -64;

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
                        ap->nAction = (nMessage == 0xA0000) + 11;
                        ap->nFrame = 0;
                    }
                }
            }

            return;
        }
        }
}
END_PS_NS
