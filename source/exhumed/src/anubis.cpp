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
#include "anubis.h"
#include "aistuff.h"
#include "engine.h"
#include "runlist.h"
#include "sequence.h"
#include "move.h"
#include "bullet.h"
#include "random.h"
#include "items.h"
#include "object.h"
#include "sound.h"
#include "trigdat.h"
#include <assert.h>

BEGIN_PS_NS

#define kMaxAnubis	80

struct Anubis
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short f;
    short g;
    short h;
};

Anubis AnubisList[kMaxAnubis];

short AnubisCount  = -1;

static actionSeq ActionSeq[] = {
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

short nAnubisDrum = 0;

static SavegameHelper sgh("anubis",
    SA(AnubisList),
    SV(AnubisCount),
    SV(nAnubisDrum),
    nullptr);


void InitAnubis()
{
    AnubisCount = kMaxAnubis;
    nAnubisDrum = 1;
}

int BuildAnubis(int nSprite, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer)
{
    AnubisCount--;
    short nAnubis = AnubisCount;

    if (nAnubis < 0) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 101);
    }
    else
    {
        changespritestat(nSprite, 101);

        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sector[sprite[nSprite].sectnum].floorz;
        nAngle = sprite[nSprite].ang;
    }

    assert(nSprite >=0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].shade = -12;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].clipdist = 60;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

    if (bIsDrummer)
    {
        AnubisList[nAnubis].nAction = nAnubisDrum + 6;
        nAnubisDrum++;

        if (nAnubisDrum >= 5) {
            nAnubisDrum = 0;
        }
    }
    else
    {
        AnubisList[nAnubis].nAction = 0;
    }

    AnubisList[nAnubis].nHealth = 540;
    AnubisList[nAnubis].nFrame  = 0;
    AnubisList[nAnubis].nSprite = nSprite;
    AnubisList[nAnubis].nTarget = -1;
    AnubisList[nAnubis].g = 0;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nAnubis | 0x90000);

    runlist_AddRunRec(NewRun, nAnubis | 0x90000);
    nCreaturesLeft++;

    return nAnubis | 0x90000;
}

void FuncAnubis(int a, int nDamage, int nRun)
{
    short nAnubis = RunData[nRun].nVal;
    assert(nAnubis >= 0 && nAnubis < kMaxAnubis);

    short nSprite = AnubisList[nAnubis].nSprite;
    short nAction = AnubisList[nAnubis].nAction;

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

            short nSeq = SeqOffsets[kSeqAnubis] + ActionSeq[nAction].a;

            seq_MoveSequence(nSprite, nSeq, AnubisList[nAnubis].nFrame);

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, AnubisList[nAnubis].nFrame);

            AnubisList[nAnubis].nFrame++;
            if (AnubisList[nAnubis].nFrame >= SeqSize[nSeq])
            {
                AnubisList[nAnubis].nFrame = 0;
                bVal = true;
            }

            short nTarget = AnubisList[nAnubis].nTarget;

            short nFrame = SeqBase[nSeq] + AnubisList[nAnubis].nFrame;
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
                            AnubisList[nAnubis].nAction = 1;
                            AnubisList[nAnubis].nFrame = 0;
                            AnubisList[nAnubis].nTarget = nTarget;

                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> 2;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
                        }
                    }
                    return;
                }
                case 1:
                {
                    if ((nAnubis & 0x1F) == (totalmoves & 0x1F))
                    {
                        PlotCourseToSprite(nSprite, nTarget);

                        int nAngle = sprite[nSprite].ang & 0xFFF8;
                        sprite[nSprite].xvel = Cos(nAngle) >> 2;
                        sprite[nSprite].yvel = Sin(nAngle) >> 2;
                    }

                    switch (nMov & 0xC000)
                    {
                        case 0xC000:
                        {
                            if ((nMov & 0x3FFF) == nTarget)
                            {
                                int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                                int nAngDiff = AngleDiff(sprite[nSprite].ang, nAng);

                                if (nAngDiff < 64)
                                {
                                    AnubisList[nAnubis].nAction = 2;
                                    AnubisList[nAnubis].nFrame = 0;
                                }
                                break;
                            }
                            // else we fall through to 0x8000
                            fallthrough__;
                        }
                        case 0x8000:
                        {
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> 2;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
                            break;
                        }

                        default:
                        {
                            if (AnubisList[nAnubis].g)
                            {
                                AnubisList[nAnubis].g--;
                            }
                            else
                            {
                                AnubisList[nAnubis].g = 60;

                                if (nTarget > -1) // NOTE: nTarget can be -1. this check wasn't in original code. TODO: demo compatiblity?
                                {
                                    if (cansee(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - GetSpriteHeight(nSprite), sprite[nSprite].sectnum,
                                        sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
                                    {
                                        sprite[nSprite].xvel = 0;
                                        sprite[nSprite].yvel = 0;
                                        sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                                        AnubisList[nAnubis].nAction = 3;
                                        AnubisList[nAnubis].nFrame = 0;
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
                        AnubisList[nAnubis].nAction = 0;
                        AnubisList[nAnubis].g = 50;
                    }
                    else
                    {
                        if (PlotCourseToSprite(nSprite, nTarget) >= 768)
                        {
                            AnubisList[nAnubis].nAction = 1;
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
                        AnubisList[nAnubis].nAction = 1;

                        sprite[nSprite].xvel = Cos(sprite[nSprite].ang) >> 2;
                        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
                        AnubisList[nAnubis].nFrame = 0;
                    }
                    else
                    {
                        // loc_25718:
                        if (nFlag & 0x80)
                        {
                            BuildBullet(nSprite, 8, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1);
                        }
                    }

                    return;
                }
                case 4:
                case 5:
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;

                    if (bVal)
                    {
                        AnubisList[nAnubis].nAction = 1;
                        AnubisList[nAnubis].nFrame = 0;
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
                        AnubisList[nAnubis].nAction = (RandomSize(3) % 5) + 6;
                        AnubisList[nAnubis].nFrame = 0;
                    }
                    return;
                }
                case 11:
                case 12:
                {
                    if (bVal)
                    {
                        AnubisList[nAnubis].nAction = nAction + 2;
                        AnubisList[nAnubis].nFrame = 0;

                        sprite[nSprite].xvel = 0;
                        sprite[nSprite].yvel = 0;
                    }
                    return;
                }
                case 13:
                case 14:
                {
                    sprite[nSprite].cstat &= 0xFEFE;
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
                    AnubisList[nAnubis].nAction = 0;
                    AnubisList[nAnubis].nFrame = 0;
                    AnubisList[nAnubis].g = 100;
                    AnubisList[nAnubis].nTarget = -1;

                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                }
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqAnubis] + ActionSeq[nAction].a, AnubisList[nAnubis].nFrame, ActionSeq[nAction].b);
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
                if (AnubisList[nAnubis].nHealth <= 0)
                    return;

                AnubisList[nAnubis].nHealth -= nDamage;

                if (AnubisList[nAnubis].nHealth > 0)
                {
                    short nTarget = a & 0xFFFF;

                    // loc_258D6:
                    if (nTarget < 0) {
                        return;
                    }

                    if (sprite[nTarget].statnum == 100 || sprite[nTarget].statnum < 199)
                    {
                        if (!RandomSize(5)) {
                            AnubisList[nAnubis].nTarget = nTarget;
                        }
                    }

                    if (RandomSize(1))
                    {
                        if (nAction >= 6 && nAction <= 10)
                        {
                            int nDrumSprite = insertsprite(sprite[nSprite].sectnum, kStatAnubisDrum);

                            sprite[nDrumSprite].x = sprite[nSprite].x;
                            sprite[nDrumSprite].y = sprite[nSprite].y;
                            sprite[nDrumSprite].z = sector[sprite[nDrumSprite].sectnum].floorz;
                            sprite[nDrumSprite].xrepeat = 40;
                            sprite[nDrumSprite].yrepeat = 40;
                            sprite[nDrumSprite].shade = -64;

                            BuildObject(nDrumSprite, 2, 0);
                        }

                        AnubisList[nAnubis].nAction = 4;
                        AnubisList[nAnubis].nFrame = 0;
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
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                    sprite[nSprite].z = sector[sprite[nSprite].sectnum].floorz;
                    sprite[nSprite].cstat &= 0xFEFE;

                    AnubisList[nAnubis].nHealth = 0;

                    nCreaturesLeft--;

                    if (nAction < 11)
                    {
                        DropMagic(nSprite);
                        AnubisList[nAnubis].nAction = (nMessage == 0xA0000) + 11;
                        AnubisList[nAnubis].nFrame = 0;
                    }
                }
            }

            return;
        }
        }
}
END_PS_NS
