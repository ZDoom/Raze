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

int BuildScorp(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    auto nScorp = scorpion.Reserve(1);

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 122);
    }
    else
    {
        changespritestat(nSprite, 122);

        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sector[sprite[nSprite].sectnum].floorz;
        nAngle = sprite[nSprite].ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].clipdist = 70;
    sprite[nSprite].shade = -12;
    sprite[nSprite].xrepeat = 80;
    sprite[nSprite].yrepeat = 80;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;
    sprite[nSprite].hitag = 0;

//	GrabTimeSlot(3);

    scorpion[nScorp].nHealth = 20000;
    scorpion[nScorp].nFrame = 0;
    scorpion[nScorp].nAction = 0;
    scorpion[nScorp].nSprite = nSprite;
    scorpion[nScorp].nTarget = -1;
    scorpion[nScorp].nCount = 0;
    scorpion[nScorp].nIndex2 = 1;

    scorpion[nScorp].nChannel = nChannel;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nScorp | 0x220000);
    scorpion[nScorp].nRun = runlist_AddRunRec(NewRun, nScorp | 0x220000);

    nCreaturesTotal++;

    return nScorp | 0x220000;
}

void FuncScorp(int a, int nDamage, int nRun)
{
    short nScorp = RunData[nRun].nVal;
    assert(nScorp >= 0 && nScorp < (int)scorpion.Size());

    short nSprite = scorpion[nScorp].nSprite;
    short nAction = scorpion[nScorp].nAction;

    bool bVal = false;

    short nTarget = -1;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Scorp\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a, scorpion[nScorp].nFrame, ScorpSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            nDamage = runlist_CheckRadialDamage(nSprite);
            if (!nDamage) {
                return;
            }
            // else fall through to case 0x80000
            fallthrough__;
        }

        case 0x80000:
        {
            if (scorpion[nScorp].nHealth <= 0) {
                return;
            }

            scorpion[nScorp].nHealth -= nDamage;

            if (scorpion[nScorp].nHealth <= 0)
            {
                scorpion[nScorp].nHealth = 0;
                scorpion[nScorp].nAction = 4;
                scorpion[nScorp].nFrame = 0;
                scorpion[nScorp].nCount = 10;

                sprite[nSprite].xvel = 0;
                sprite[nSprite].yvel = 0;
                sprite[nSprite].zvel = 0;
                sprite[nSprite].cstat &= 0xFEFE;

                nCreaturesKilled++;
                return;
            }
            else
            {
                nTarget = a & 0xFFFF;

                if (nTarget >= 0)
                {
                    if (sprite[nSprite].statnum == 100 || (sprite[nSprite].statnum < 199 && !RandomSize(5)))
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

                goto FS_Pink_A;
            }
        }

        case 0x20000:
        {
            if (scorpion[nScorp].nHealth) {
                Gravity(nSprite);
            }

            int nSeq = SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, scorpion[nScorp].nFrame);
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
                                sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
                                sprite[nSprite].yvel = bsin(sprite[nSprite].ang);

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
                        // GOTO FS_Pink_A:
                        goto FS_Pink_A;
                    }
                    else
                    {
                        int nMov = MoveCreatureWithCaution(nSprite);
                        if ((nMov & 0xC000) == 0xC000)
                        {
                            if (nTarget == (nMov & 0x3FFF))
                            {
                                int nAngle = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                                if (AngleDiff(sprite[nSprite].ang, nAngle) < 64)
                                {
                                    scorpion[nScorp].nAction = 2;
                                    scorpion[nScorp].nFrame = 0;
                                }

                                goto FS_Red;
                            }
                            else
                            {
                                goto FS_Pink_A;
                            }
                        }
                        else if ((nMov & 0xC000) == 0x8000)
                        {
                            goto FS_Pink_A;
                        }
                        else
                        {
                            goto FS_Pink_B;
                        }
                    }
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

                    goto FS_Red;
                }

                case 3:
                {
                    if (bVal)
                    {
                        scorpion[nScorp].nIndex--;
                        if (scorpion[nScorp].nIndex <= 0)
                        {
                            scorpion[nScorp].nAction = 1;

                            sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
                            sprite[nSprite].yvel = bsin(sprite[nSprite].ang);

                            scorpion[nScorp].nFrame = 0;
                            return;
                        }
                    }

                    if (!(nFlag & 0x80)) {
                        return;
                    }

                    short nBulletSprite = BuildBullet(nSprite, 16, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1) & 0xFFFF;
                    if (nBulletSprite > -1)
                    {
                        PlotCourseToSprite(nBulletSprite, nTarget);
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

                    int nSpider = BuildSpider(-1, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].ang);
                    if (nSpider != -1)
                    {
                        short nSpiderSprite = nSpider & 0xFFFF;

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
                    sprite[nSprite].cstat &= 0xFEFE;

                    if (bVal)
                    {
                        runlist_SubRunRec(scorpion[nScorp].nRun);
                        runlist_DoSubRunRec(sprite[nSprite].owner);
                        runlist_FreeRun(sprite[nSprite].lotag - 1);

                        mydeletesprite(nSprite);
                    }

                    return;
                }
            }

            break;
        }
    }

FS_Pink_A:
    PlotCourseToSprite(nSprite, nTarget);
    sprite[nSprite].ang += RandomSize(7) - 63;
    sprite[nSprite].ang &= kAngleMask;

    sprite[nSprite].xvel = bcos(sprite[nSprite].ang);
    sprite[nSprite].yvel = bsin(sprite[nSprite].ang);

FS_Pink_B:
    if (scorpion[nScorp].nCount)
    {
        scorpion[nScorp].nCount--;
    }
    else
    {
        scorpion[nScorp].nCount = 45;

        if (cansee(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - GetSpriteHeight(nSprite), sprite[nSprite].sectnum,
            sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
        {
            sprite[nSprite].xvel = 0;
            sprite[nSprite].yvel = 0;
            sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

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

FS_Red:
    if (!nAction || nTarget == -1) {
        return;
    }

    if (!(sprite[nTarget].cstat & 0x101))
    {
        scorpion[nScorp].nAction = 0;
        scorpion[nScorp].nFrame = 0;
        scorpion[nScorp].nCount = 30;
        scorpion[nScorp].nTarget = -1;

        sprite[nSprite].xvel = 0;
        sprite[nSprite].yvel = 0;
    }
}
END_PS_NS
