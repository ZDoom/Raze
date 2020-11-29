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
#include "exhumed.h"
#include "engine.h"
#include "sequence.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

struct Spider
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nRun;
};

static TArray<Spider> SpiderList;

static actionSeq SpiderSeq[] = {
    {16, 0},
    {8,  0},
    {32, 0},
    {24, 0},
    {0,  0},
    {40, 1},
    {41, 1}
};

FSerializer& Serialize(FSerializer& arc, const char* keyname, Spider& w, Spider* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("run", w.nRun)
            .EndObject();
    }
    return arc;
}

void SerializeSpider(FSerializer& arc)
{
    arc("spider", SpiderList);
}

void InitSpider()
{
    SpiderList.Clear();
}

int BuildSpider(int nSprite, int x, int y, int z, short nSector, int nAngle)
{
    auto nSpider = SpiderList.Reserve(1);
    auto spp = &SpiderList[nSpider];

    spritetype* sp;
    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 99);
        sp = &sprite[nSprite];
    }
    else
    {
        changespritestat(nSprite, 99);
        sp = &sprite[nSprite];

        x = sp->x;
        y = sp->y;
        z = sector[sp->sectnum].floorz;
        nAngle = sp->ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0x101;
    sp->shade = -12;
    sp->clipdist = 15;
    sp->xvel = 0;
    sp->yvel = 0;
    sp->zvel = 0;
    sp->xrepeat = 40;
    sp->yrepeat = 40;
    sp->pal = sector[sp->sectnum].ceilingpal;
    sp->xoffset = 0;
    sp->yoffset = 0;
    sp->ang = nAngle;
    sp->picnum = 1;
    sp->hitag = 0;
    sp->lotag = runlist_HeadRun() + 1;
    sp->extra = -1;

    //	GrabTimeSlot(3);

    spp->nAction = 0;
    spp->nFrame = 0;
    spp->nSprite = nSprite;
    spp->nTarget = -1;
    spp->nHealth = 160;

    sp->owner = runlist_AddRunRec(sp->lotag - 1, nSpider | 0xC0000);

    spp->nRun = runlist_AddRunRec(NewRun, nSpider | 0xC0000);

    nCreaturesTotal++;

    return nSpider | 0xC0000;
}

void FuncSpider(int a, int nDamage, int nRun)
{
    int nSpider = RunData[nRun].nVal;
    auto spp = &SpiderList[nSpider];
    assert(nSpider >= 0 && nSpider < (int)SpiderList.Size());

    int nVel = 0;

    int nSprite = spp->nSprite;
    auto sp = &sprite[nSprite];
    short nAction = spp->nAction;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Spider\n", nMessage);
            break;
        }

        case 0x20000:
        {
            nVel = 6;

            if (spp->nHealth)
            {
                if (sp->cstat & 8)
                {
                    sp->z = sector[sp->sectnum].ceilingz + GetSpriteHeight(nSprite);
                }
                else
                {
                    Gravity(nSprite);
                }
            }

            int nSeq = SeqOffsets[kSeqSpider] + SpiderSeq[nAction].a;

            sp->picnum = seq_GetSeqPicnum2(nSeq, spp->nFrame);

            seq_MoveSequence(nSprite, nSeq, spp->nFrame);

            int nFrameFlag = FrameFlag[SeqBase[nSeq] + spp->nFrame];

            spp->nFrame++;
            if (spp->nFrame >= SeqSize[nSeq]) {
                spp->nFrame = 0;
            }

            short nTarget = spp->nTarget;

            if (nTarget <= -1 || sprite[nTarget].cstat & 0x101)
            {
                switch (nAction)
                {
                default:
                    return;

                case 0:
                {
                    if ((nSpider & 0x1F) == (totalmoves & 0x1F))
                    {
                        if (nTarget < 0) {
                            nTarget = FindPlayer(nSprite, 100);
                        }

                        if (nTarget >= 0)
                        {
                            spp->nAction = 1;
                                spp->nFrame = 0;
                            spp->nTarget = nTarget;

                            sp->xvel = bcos(sp->ang);
                            sp->yvel = bsin(sp->ang);
                            return;
                        }
                    }

                    break;
                }
                case 1:
                {
                    if (nTarget >= 0) {
                            nVel++;
                    }
    goto case_3;
                    break;
                }
                case 4:
                {
                        if (!spp->nFrame)
                    {
                            spp->nFrame  = 0;
                        spp->nAction = 1;
                    }
                    fallthrough__;
                }
                case 3:
                {
    case_3:
                    short nSector = sp->sectnum;

                    if (sp->cstat & 8)
                    {
                        sp->zvel = 0;
                        sp->z = sector[nSector].ceilingz + (tileHeight(sp->picnum) << 5);

                        if (sector[nSector].ceilingstat & 1)
                        {
                            sp->cstat ^= 8;
                            sp->zvel = 1;

                            spp->nAction = 3;
                                spp->nFrame  = 0;
                        }
                    }

                    if ((totalmoves & 0x1F) == (nSpider & 0x1F))
                    {
                        PlotCourseToSprite(nSprite, nTarget);

                        if (RandomSize(3))
                        {
                            sp->xvel = bcos(sp->ang);
                            sp->yvel = bsin(sp->ang);
                        }
                        else
                        {
                            sp->xvel = 0;
                            sp->yvel = 0;
                        }

                        if (spp->nAction == 1 && RandomBit())
                        {
                            if (sp->cstat & 8)
                            {
                                    sp->cstat ^= 8;
                                sp->zvel = 1;
                                sp->z = sector[nSector].ceilingz + GetSpriteHeight(nSprite);
                            }
                            else
                            {
                                sp->zvel = -5120;
                            }

                            spp->nAction = 3;
                                spp->nFrame = 0;

                            if (!RandomSize(3)) {
                                D3PlayFX(StaticSound[kSound29], nSprite);
                            }
                        }
                    }
                    break;
                }
                case 5:
                {
                        if (!spp->nFrame)
                    {
                        runlist_DoSubRunRec(sp->owner);
                        runlist_FreeRun(sp->lotag - 1);
                            runlist_SubRunRec(spp->nRun);
                        sp->cstat = 0x8000;
                        mydeletesprite(nSprite);
                    }
                    return;
                }
                case 2:
                {
                    if (nTarget != -1)
                    {
                        if (nFrameFlag & 0x80)
                        {
                            runlist_DamageEnemy(nTarget, nSprite, 3);
                            D3PlayFX(StaticSound[kSound38], nSprite);
                        }

                        if (PlotCourseToSprite(nSprite, nTarget) < 1024) {
                            return;
                        }

                        spp->nAction = 1;
                    }
                    else
                    {
                        spp->nAction = 0;
                        sp->xvel = 0;
                        sp->yvel = 0;
                    }

                        spp->nFrame = 0;
                    break;
                }
                }
            }
            else
            {
                spp->nTarget = -1;
                spp->nAction = 0;
                spp->nFrame = 0;

                sp->xvel = 0;
                sp->yvel = 0;
            }

            int nMov = movesprite(nSprite, sp->xvel << nVel, sp->yvel << nVel, sp->zvel, 1280, -1280, CLIPMASK0);

            if (!nMov)
                return;

            if (nMov & 0x10000
                && sp->zvel < 0
                && (hihit & 0xC000) != 0xC000
                && !((sector[sp->sectnum].ceilingstat) & 1))
            {
                sp->cstat |= 8;
                sp->z = GetSpriteHeight(nSprite) + sector[sp->sectnum].ceilingz;
                sp->zvel = 0;

                spp->nAction = 1;
                spp->nFrame = 0;
                return;
            }
            else
            {
                switch (nMov & 0xC000)
                {
                case 0x8000:
                {
                    sp->ang = (sp->ang + 256) & 0x7EF;
                    sp->xvel = bcos(sp->ang);
                    sp->yvel = bsin(sp->ang);
                    return;
                }
                case 0xC000:
                {
                    if ((nMov & 0x3FFF) == nTarget)
                    {
                        int nAng = getangle(sprite[nTarget].x - sp->x, sprite[nTarget].y - sp->y);
                        if (AngleDiff(sp->ang, nAng) < 64)
                        {
                            spp->nAction = 2;
                                spp->nFrame = 0;
                        }
                    }
                    return;
                }
                default:
                    break;
                }

                if (spp->nAction == 3)
                {
                    spp->nAction = 1;
                    spp->nFrame = 0;
                }
                return;
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqSpider] + SpiderSeq[nAction].a, spp->nFrame, SpiderSeq[nAction].b);
            break;
        }

        case 0xA0000:
        {
            if (spp->nHealth <= 0)
                return;

            nDamage = runlist_CheckRadialDamage(nSprite);
            // fall through
            fallthrough__;
        }

        case 0x80000:
        {
            if (!nDamage)
                return;

            short nTarget = a & 0xFFFF;

            spp->nHealth -= nDamage;
            if (spp->nHealth > 0)
            {
                /*
                NOTE:
                    nTarget check was added, but should we return if it's invalid instead
                    or should code below (action set, b set) happen?
                    Other AI doesn't show consistency in this regard (see Scorpion code)
                */
                if (nTarget > -1 && sprite[nTarget].statnum == 100)
                {
                    spp->nTarget = nTarget;
                }

                spp->nAction = 4;
                spp->nFrame  = 0;
            }
            else
            {
                // creature is dead, make some chunks
                spp->nHealth = 0;
                spp->nAction = 5;
                spp->nFrame  = 0;

                sp->cstat &= 0xFEFE;

                nCreaturesKilled++;

                for (int i = 0; i < 7; i++)
                {
                    BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqSpider, i + 41, 0));
                }
            }

            return;
        }
        }
}
END_PS_NS
