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

short SpiderCount = 0;

#define kMaxSpiders		100

struct Spider
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nRun;
};

Spider SpiderList[kMaxSpiders];

static actionSeq ActionSeq[] = {
    {16, 0},
    {8,  0},
    {32, 0},
    {24, 0},
    {0,  0},
    {40, 1},
    {41, 1}
};

static SavegameHelper sgh("spider",
    SV(SpiderCount),
    SA(SpiderList),
    nullptr);

void InitSpider()
{
    SpiderCount = 0;
}

int BuildSpider(int nSprite, int x, int y, int z, short nSector, int nAngle)
{
    int nSpider = SpiderCount++;
    if (nSpider >= kMaxSpiders) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 99);
    }
    else
    {
        changespritestat(nSprite, 99);
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sprite[nSprite].z;
        nAngle = sprite[nSprite].ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].shade = -12;
    sprite[nSprite].clipdist = 15;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;

    //	GrabTimeSlot(3);

    SpiderList[nSpider].nAction = 0;
    SpiderList[nSpider].nFrame = 0;
    SpiderList[nSpider].nSprite = nSprite;
    SpiderList[nSpider].nTarget = -1;
    SpiderList[nSpider].nHealth = 160;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSpider | 0xC0000);

    SpiderList[nSpider].nRun = runlist_AddRunRec(NewRun, nSpider | 0xC0000);

    nCreaturesTotal++;

    return nSpider | 0xC0000;
}

void FuncSpider(int a, int nDamage, int nRun)
{
    short nSpider = RunData[nRun].nVal;
    assert(nSpider >= 0 && nSpider < kMaxSpiders);

    int nVel = 0;

    short nSprite = SpiderList[nSpider].nSprite;
    short nAction = SpiderList[nSpider].nAction;

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

            if (SpiderList[nSpider].nHealth)
            {
                if (sprite[nSprite].cstat & 8)
                {
                    sprite[nSprite].z = sector[sprite[nSprite].sectnum].ceilingz + GetSpriteHeight(nSprite);
                }
                else
                {
                    Gravity(nSprite);
                }
            }

            int nSeq = SeqOffsets[kSeqSpider] + ActionSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, SpiderList[nSpider].nFrame);

            seq_MoveSequence(nSprite, nSeq, SpiderList[nSpider].nFrame);

            int nFrameFlag = FrameFlag[SeqBase[nSeq] + SpiderList[nSpider].nFrame];

            SpiderList[nSpider].nFrame++;
            if (SpiderList[nSpider].nFrame >= SeqSize[nSeq]) {
                SpiderList[nSpider].nFrame = 0;
            }

            short nTarget = SpiderList[nSpider].nTarget;

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
                            SpiderList[nSpider].nAction = 1;
                                SpiderList[nSpider].nFrame = 0;
                            SpiderList[nSpider].nTarget = nTarget;

                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang);
                            sprite[nSprite].yvel = sintable[sprite[nSprite].ang]; // NOTE - not angle masking here in original code
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
                        if (!SpiderList[nSpider].nFrame)
                    {
                            SpiderList[nSpider].nFrame  = 0;
                        SpiderList[nSpider].nAction = 1;
                    }
                    fallthrough__;
                }
                case 3:
                {
    case_3:
                    short nSector = sprite[nSprite].sectnum;

                    if (sprite[nSprite].cstat & 8)
                    {
                        sprite[nSprite].zvel = 0;
                        sprite[nSprite].z = sector[nSector].ceilingz + (tilesiz[sprite[nSprite].picnum].y << 5);

                        if (sector[nSector].ceilingstat & 1)
                        {
                            sprite[nSprite].cstat ^= 8;
                            sprite[nSprite].zvel = 1;

                            SpiderList[nSpider].nAction = 3;
                                SpiderList[nSpider].nFrame  = 0;
                        }
                    }

                    if ((totalmoves & 0x1F) == (nSpider & 0x1F))
                    {
                        PlotCourseToSprite(nSprite, nTarget);

                        if (RandomSize(3))
                        {
                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang);
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang);
                        }
                        else
                        {
                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
                        }

                        if (SpiderList[nSpider].nAction == 1 && RandomBit())
                        {
                            if (sprite[nSprite].cstat & 8)
                            {
                                    sprite[nSprite].cstat ^= 8;
                                sprite[nSprite].zvel = 1;
                                sprite[nSprite].z = sector[nSector].ceilingz + GetSpriteHeight(nSprite);
                            }
                            else
                            {
                                sprite[nSprite].zvel = -5120;
                            }

                            SpiderList[nSpider].nAction = 3;
                                SpiderList[nSpider].nFrame = 0;

                            if (!RandomSize(3)) {
                                D3PlayFX(StaticSound[kSound29], nSprite);
                            }
                        }
                    }
                    break;
                }
                case 5:
                {
                        if (!SpiderList[nSpider].nFrame)
                    {
                        runlist_DoSubRunRec(sprite[nSprite].owner);
                        runlist_FreeRun(sprite[nSprite].lotag - 1);
                            runlist_SubRunRec(SpiderList[nSpider].nRun);
                        sprite[nSprite].cstat = 0x8000;
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

                        SpiderList[nSpider].nAction = 1;
                    }
                    else
                    {
                        SpiderList[nSpider].nAction = 0;
                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                    }

                        SpiderList[nSpider].nFrame = 0;
                    break;
                }
                }
            }
            else
            {
                SpiderList[nSpider].nTarget = -1;
                SpiderList[nSpider].nAction = 0;
                SpiderList[nSpider].nFrame = 0;

                sprite[nSprite].xvel = 0;
                sprite[nSprite].yvel = 0;
            }

            int nMov = movesprite(nSprite, sprite[nSprite].xvel << nVel, sprite[nSprite].yvel << nVel, sprite[nSprite].zvel, 1280, -1280, CLIPMASK0);

            if (!nMov)
                return;

            if (nMov & 0x10000
                && sprite[nSprite].zvel < 0
                && (hihit & 0xC000) != 0xC000
                && !((sector[sprite[nSprite].sectnum].ceilingstat) & 1))
            {
                sprite[nSprite].cstat |= 8;
                sprite[nSprite].z = GetSpriteHeight(nSprite) + sector[sprite[nSprite].sectnum].ceilingz;
                sprite[nSprite].zvel = 0;

                SpiderList[nSpider].nAction = 1;
                SpiderList[nSpider].nFrame = 0;
                return;
            }
            else
            {
                switch (nMov & 0xC000)
                {
                case 0x8000:
                {
                    sprite[nSprite].ang = (sprite[nSprite].ang + 256) & 0x7EF;
                    sprite[nSprite].xvel = Cos(sprite[nSprite].ang);
                    sprite[nSprite].yvel = Sin(sprite[nSprite].ang);
                    return;
                }
                case 0xC000:
                {
                    if ((nMov & 0x3FFF) == nTarget)
                    {
                        int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                        if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                        {
                            SpiderList[nSpider].nAction = 2;
                                SpiderList[nSpider].nFrame = 0;
                        }
                    }
                    return;
                }
                default:
                    break;
                }

                if (SpiderList[nSpider].nAction == 3)
                {
                    SpiderList[nSpider].nAction = 1;
                    SpiderList[nSpider].nFrame = 0;
                }
                return;
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqSpider] + ActionSeq[nAction].a, SpiderList[nSpider].nFrame, ActionSeq[nAction].b);
            break;
        }

        case 0xA0000:
        {
            if (SpiderList[nSpider].nHealth <= 0)
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

            SpiderList[nSpider].nHealth -= nDamage;
            if (SpiderList[nSpider].nHealth > 0)
            {
                /*
                NOTE:
                    nTarget check was added, but should we return if it's invalid instead
                    or should code below (action set, b set) happen?
                    Other AI doesn't show consistency in this regard (see Scorpion code)
                */
                if (nTarget > -1 && sprite[nTarget].statnum == 100)
                {
                    SpiderList[nSpider].nTarget = nTarget;
                }

                SpiderList[nSpider].nAction = 4;
                SpiderList[nSpider].nFrame  = 0;
            }
            else
            {
                // creature is dead, make some chunks
                SpiderList[nSpider].nHealth = 0;
                SpiderList[nSpider].nAction = 5;
                SpiderList[nSpider].nFrame  = 0;

                sprite[nSprite].cstat &= 0xFEFE;

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
