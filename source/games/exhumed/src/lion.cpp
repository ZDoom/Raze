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

static actionSeq LionSeq[] = {
    {54, 1},
    {18, 0},
    {0,  0},
    {10, 0},
    {44, 0},
    {18, 0},
    {26, 0},
    {34, 0},
    {8,  1},
    {9,  1},
    {52, 1},
    {53, 1}
};

struct Lion
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

TArray<Lion> LionList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Lion& w, Lion* def)
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

void SerializeLion(FSerializer& arc)
{
    arc("lion", LionList);
}

void InitLion()
{
    LionList.Clear();
}

int BuildLion(short nSprite, int x, int y, int z, short nSector, short nAngle)
{
    auto nLion = LionList.Reserve(1);

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 104);
    }
    else
    {
        changespritestat(nSprite, 104);
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
    sprite[nSprite].clipdist = 60;
    sprite[nSprite].shade = -12;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

    LionList[nLion].nAction = 0;
    LionList[nLion].nHealth = 500;
    LionList[nLion].nFrame = 0;
    LionList[nLion].nSprite = nSprite;
    LionList[nLion].nTarget = -1;
    LionList[nLion].nCount = 0;
    LionList[nLion].nIndex = nLion;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nLion | 0x130000);

    LionList[nLion].nRun = runlist_AddRunRec(NewRun, nLion | 0x130000);

    nCreaturesTotal++;

    return nLion | 0x130000;
}

void FuncLion(int a, int nDamage, int nRun)
{
    short nLion = RunData[nRun].nVal;
    assert(nLion >= 0 && nLion < (int)LionList.Size());

    short nSprite = LionList[nLion].nSprite;
    short nAction = LionList[nLion].nAction;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Lion\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a, SeqOffsets[kSeqLion] + LionSeq[nAction].a, LionList[nLion].nFrame, LionSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            nDamage = runlist_CheckRadialDamage(nSprite);
            // now fall through to 0x80000
            fallthrough__;
        }
        case 0x80000:
        {
            if (nDamage && LionList[nLion].nHealth > 0)
            {
                LionList[nLion].nHealth -= dmgAdjust(nDamage);
                if (LionList[nLion].nHealth <= 0)
                {
                    // R.I.P.
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                    sprite[nSprite].cstat &= 0xFEFE;

                    LionList[nLion].nHealth = 0;

                    nCreaturesKilled++;

                    if (nAction < 10)
                    {
                        DropMagic(nSprite);

                        if (nMessage == 0xA0000) {
                            LionList[nLion].nAction = 11;
                        }
                        else
                        {
                            LionList[nLion].nAction = 10;
                        }

                        LionList[nLion].nFrame = 0;
                        return;
                    }
                }
                else
                {
                    short nTarget = a & 0xFFFF;

                    if (nTarget > -1)
                    {
                        if (sprite[nTarget].statnum < 199) {
                            LionList[nLion].nTarget = nTarget;
                        }

                        if (nAction != 6)
                        {
                            if (RandomSize(8) <= (LionList[nLion].nHealth >> 2))
                            {
                                LionList[nLion].nAction = 4;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                            }
                            else if (RandomSize(1))
                            {
                                PlotCourseToSprite(nSprite, nTarget);
                                LionList[nLion].nAction = 5;
                                LionList[nLion].nCount = RandomSize(3);
                                sprite[nSprite].ang = (sprite[nSprite].ang - (RandomSize(1) << 8)) + (RandomSize(1) << 8); // NOTE: no angle mask in original code
                            }
                            else
                            {
                                LionList[nLion].nAction = 8;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                                sprite[nSprite].cstat &= 0xFEFE;
                            }

                            LionList[nLion].nFrame = 0;
                        }
                    }
                }
            }
            return;
        }

        case 0x20000:
        {
            if (nAction != 7) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqLion] + LionSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, LionList[nLion].nFrame);

            seq_MoveSequence(nSprite, nSeq, LionList[nLion].nFrame);

            LionList[nLion].nFrame++;
            if (LionList[nLion].nFrame >= SeqSize[nSeq])
            {
                LionList[nLion].nFrame = 0;
                bVal = true;
            }

            short nFlag = FrameFlag[SeqBase[nSeq] + LionList[nLion].nFrame];
            short nTarget = LionList[nLion].nTarget;

            int nMov = MoveCreatureWithCaution(nSprite);

            switch (nAction)
            {
                default:
                    return;

                case 0:
                case 1:
                {
                    if ((LionList[nLion].nIndex & 0x1F) == (totalmoves & 0x1F))
                    {
                        if (nTarget < 0)
                        {
                            nTarget = FindPlayer(nSprite, 40);
                            if (nTarget >= 0)
                            {
                                D3PlayFX(StaticSound[kSound24], nSprite);
                                LionList[nLion].nAction = 2;
                                LionList[nLion].nFrame = 0;

                                sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -1);
                                sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -1);
                                LionList[nLion].nTarget = nTarget;
                                return;
                            }
                        }
                    }

                    if (nAction && !easy())
                    {
                        LionList[nLion].nCount--;
                        if (LionList[nLion].nCount <= 0)
                        {
                            if (RandomBit())
                            {
                                sprite[nSprite].ang = RandomWord() & kAngleMask;
                                sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -1);
                                sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -1);
                            }
                            else
                            {
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                            }

                            LionList[nLion].nCount = 100;
                        }
                    }

                    return;
                }

                case 2:
                {
                    if ((totalmoves & 0x1F) == (LionList[nLion].nIndex & 0x1F))
                    {
                        PlotCourseToSprite(nSprite, nTarget);

                        int nAng = sprite[nSprite].ang & 0xFFF8;

                        if (sprite[nSprite].cstat & 0x8000)
                        {
                            sprite[nSprite].xvel = bcos(nAng, 1);
                            sprite[nSprite].yvel = bsin(nAng, 1);
                        }
                        else
                        {
                            sprite[nSprite].xvel = bcos(nAng, -1);
                            sprite[nSprite].yvel = bsin(nAng, -1);
                        }
                    }

                    if ((nMov & 0xC000) < 0x8000)
                    {
                        break;
                    }
                    else if ((nMov & 0xC000) == 0x8000)
                    {
                        // loc_378FA:
                        sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -1);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -1);
                        break;
                    }
                    else if ((nMov & 0xC000) == 0xC000)
                    {
                        if ((nMov & 0x3FFF) == nTarget)
                        {
                            if (sprite[nSprite].cstat & 0x8000)
                            {
                                LionList[nLion].nAction = 9;
                                sprite[nSprite].cstat &= 0x7FFF;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                            }
                            else
                            {
                                int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                                if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                                {
                                    LionList[nLion].nAction = 3;
                                }
                            }

                            LionList[nLion].nFrame = 0;
                            break;
                        }
                        else
                        {
                            // loc_378FA:
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -1);
                            sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -1);
                            break;
                        }
                    }

                    break;
                }

                case 3:
                {
                    if (nTarget == -1)
                    {
                        LionList[nLion].nAction = 1;
                        LionList[nLion].nCount = 50;
                    }
                    else
                    {
                        if (PlotCourseToSprite(nSprite, nTarget) >= 768)
                        {
                            LionList[nLion].nAction = 2;
                        }
                        else if (nFlag & 0x80)
                        {
                            runlist_DamageEnemy(nTarget, nSprite, 10);
                        }
                    }

                    break;
                }

                case 4:
                {
                    if (bVal)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion].nFrame = 0;
                    }

                    if (nMov & 0x20000)
                    {
                        sprite[nSprite].xvel >>= 1;
                        sprite[nSprite].yvel >>= 1;
                    }

                    return;
                }

                case 5: // Jump away when damaged
                {
                    LionList[nLion].nCount--;
                    if (LionList[nLion].nCount <= 0)
                    {
                        sprite[nSprite].zvel = -4000;
                        LionList[nLion].nCount = 0;

                        int x = sprite[nSprite].x;
                        int y = sprite[nSprite].y;
                        int z = sprite[nSprite].z - (GetSpriteHeight(nSprite) >> 1);

                        int nCheckDist = 0x7FFFFFFF;

                        short nAngle = sprite[nSprite].ang;
                        short nScanAngle = (sprite[nSprite].ang - 512) & kAngleMask;

                        for (int i = 0; i < 5; i++)
                        {
                            short hitwall;
                            int hitx, hity;
                            vec3_t startPos = { x, y, z };
                            hitdata_t hitData;

                            hitscan(&startPos, sprite[nSprite].sectnum, bcos(nScanAngle), bsin(nScanAngle), 0, &hitData, CLIPMASK1);

                            hitx = hitData.pos.x;
                            hity = hitData.pos.y;
                            hitwall = hitData.wall;

                            if (hitwall > -1)
                            {
                                int theX = abs(hitx - x);
                                int theY = abs(hity - y);

                                if ((theX + theY) < nCheckDist)
                                {
                                    nCheckDist = theX;
                                    nAngle = nScanAngle;
                                }
                            }

                            nScanAngle += 256;
                            nScanAngle &= kAngleMask;
                        }

                        sprite[nSprite].ang = nAngle;

                        LionList[nLion].nAction = 6;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang) - bcos(sprite[nSprite].ang, -3);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang) - bsin(sprite[nSprite].ang, -3);
                        D3PlayFX(StaticSound[kSound24], nSprite);
                    }

                    return;
                }

                case 6:
                {
                    if (nMov & 0x30000)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion].nFrame = 0;
                        return;
                    }

                    if ((nMov & 0xC000) == 0x8000)
                    {
                        LionList[nLion].nAction = 7;
                        sprite[nSprite].ang = (GetWallNormal(nMov & 0x3FFF) + 1024) & kAngleMask;
                        LionList[nLion].nCount = RandomSize(4);
                        return;
                    }
                    else if ((nMov & 0xC000) == 0xC000)
                    {
                        if ((nMov & 0x3FFF) == nTarget)
                        {
                            int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                            if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                            {
                                LionList[nLion].nAction = 3;
                                LionList[nLion].nFrame = 0;
                            }
                        }
                        else
                        {
                            // loc_378FA:
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -1);
                            sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -1);
                            break;
                        }
                    }

                    return;
                }

                case 7:
                {
                    LionList[nLion].nCount--;

                    if (LionList[nLion].nCount <= 0)
                    {
                        LionList[nLion].nCount = 0;
                        if (nTarget > -1)
                        {
                            PlotCourseToSprite(nSprite, nTarget);
                        }
                        else
                        {
                            sprite[nSprite].ang = (RandomSize(9) + (sprite[nSprite].ang + 768)) & kAngleMask;
                        }

                        sprite[nSprite].zvel = -1000;

                        LionList[nLion].nAction = 6;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang) - bcos(sprite[nSprite].ang, -3);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang) - bsin(sprite[nSprite].ang, -3);
                        D3PlayFX(StaticSound[kSound24], nSprite);
                    }

                    return;
                }

                case 8:
                {
                    if (bVal)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion].nFrame  = 0;
                        sprite[nSprite].cstat |= 0x8000;
                    }
                    return;
                }

                case 9:
                {
                    if (bVal)
                    {
                        LionList[nLion].nFrame  = 0;
                        LionList[nLion].nAction = 2;
                        sprite[nSprite].cstat |= 0x101;
                    }
                    return;
                }

                case 10:
                case 11:
                {
                    if (bVal)
                    {
                        runlist_SubRunRec(sprite[nSprite].owner);
                        runlist_SubRunRec(LionList[nLion].nRun);
                        sprite[nSprite].cstat = 0x8000;
                    }
                    return;
                }
            }

            // loc_379AD: ?
            if (nAction != 1 && nTarget != -1)
            {
                if (!(sprite[nTarget].cstat & 0x101))
                {
                    LionList[nLion].nAction = 1;
                    LionList[nLion].nFrame = 0;
                    LionList[nLion].nCount = 100;
                    LionList[nLion].nTarget = -1;
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                }
            }

            return;
        }
    }
}
END_PS_NS
