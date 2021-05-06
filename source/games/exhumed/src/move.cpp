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
#include "engine.h"
#include "exhumed.h"
#include "aistuff.h"
#include "player.h"
#include "view.h"
#include "status.h"
#include "sound.h"
#include "mapinfo.h"
#include <string.h>
#include <assert.h>


BEGIN_PS_NS

short NearSector[kMaxSectors] = { 0 };

short nPushBlocks;

// TODO - moveme?
short overridesect;
short NearCount = -1;

short nBodySprite[50];

int hihit, sprceiling, sprfloor, lohit;

enum
{
	kMaxPushBlocks	= 100,
	kMaxMoveChunks	= 75
};

// think this belongs in init.c?
BlockInfo sBlockInfo[kMaxPushBlocks];

short nChunkSprite[kMaxMoveChunks];

FSerializer& Serialize(FSerializer& arc, const char* keyname, BlockInfo& w, BlockInfo* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at8", w.field_8)
            ("sprite", w.nSprite)
            ("x", w.x)
            ("y", w.y)
            .EndObject();
    }
    return arc;
}

void SerializeMove(FSerializer& arc)
{
    if (arc.BeginObject("move"))
    {
        arc("nearcount", NearCount)
            .Array("nearsector", NearSector, NearCount)
            ("pushcount", nPushBlocks)
            .Array("blocks", sBlockInfo, nPushBlocks)
            ("chunkcount", nCurChunkNum)
            .Array("chunks", nChunkSprite, kMaxMoveChunks)
            ("overridesect", overridesect)
            ("hihit", hihit)
            ("lohit", lohit)
            ("sprceiling", sprceiling)
            ("sprfloor", sprfloor)
            .Array("bodysprite", nBodySprite, 50)
            .EndObject();
    }
}

signed int lsqrt(int a1)
{
    int v1;
    int v2;
    signed int result;

    v1 = a1;
    v2 = a1 - 0x40000000;

    result = 0;

    if (v2 >= 0)
    {
        result = 32768;
        v1 = v2;
    }
    if (v1 - ((result << 15) + 0x10000000) >= 0)
    {
        v1 -= (result << 15) + 0x10000000;
        result += 16384;
    }
    if (v1 - ((result << 14) + 0x4000000) >= 0)
    {
        v1 -= (result << 14) + 0x4000000;
        result += 8192;
    }
    if (v1 - ((result << 13) + 0x1000000) >= 0)
    {
        v1 -= (result << 13) + 0x1000000;
        result += 4096;
    }
    if (v1 - ((result << 12) + 0x400000) >= 0)
    {
        v1 -= (result << 12) + 0x400000;
        result += 2048;
    }
    if (v1 - ((result << 11) + 0x100000) >= 0)
    {
        v1 -= (result << 11) + 0x100000;
        result += 1024;
    }
    if (v1 - ((result << 10) + 0x40000) >= 0)
    {
        v1 -= (result << 10) + 0x40000;
        result += 512;
    }
    if (v1 - ((result << 9) + 0x10000) >= 0)
    {
        v1 -= (result << 9) + 0x10000;
        result += 256;
    }
    if (v1 - ((result << 8) + 0x4000) >= 0)
    {
        v1 -= (result << 8) + 0x4000;
        result += 128;
    }
    if (v1 - ((result << 7) + 4096) >= 0)
    {
        v1 -= (result << 7) + 4096;
        result += 64;
    }
    if (v1 - ((result << 6) + 1024) >= 0)
    {
        v1 -= (result << 6) + 1024;
        result += 32;
    }
    if (v1 - (32 * result + 256) >= 0)
    {
        v1 -= 32 * result + 256;
        result += 16;
    }
    if (v1 - (16 * result + 64) >= 0)
    {
        v1 -= 16 * result + 64;
        result += 8;
    }
    if (v1 - (8 * result + 16) >= 0)
    {
        v1 -= 8 * result + 16;
        result += 4;
    }
    if (v1 - (4 * result + 4) >= 0)
    {
        v1 -= 4 * result + 4;
        result += 2;
    }
    if (v1 - (2 * result + 1) >= 0)
        result += 1;

    return result;
}

void MoveThings()
{
    thinktime.Reset();
    thinktime.Clock();

    UndoFlashes();
    DoLights();

    if (nFreeze)
    {
        if (nFreeze == 1 || nFreeze == 2) {
            DoSpiritHead();
        }
    }
    else
    {
        actortime.Reset();
        actortime.Clock();
        runlist_ExecObjects();
        runlist_CleanRunRecs();
        actortime.Unclock();
    }

    MoveStatus();
    DoBubbleMachines();
    DoDrips();
    DoMovingSects();
    DoRegenerates();

    if (currentLevel->gameflags & LEVEL_EX_COUNTDOWN)
    {
        DoFinale();
        if (lCountDown < 1800 && nDronePitch < 2400 && !lFinaleStart)
        {
            nDronePitch += 64;
            BendAmbientSound();
        }
    }

    thinktime.Unclock();
}

void ResetMoveFifo()
{
    movefifoend = 0;
    movefifopos = 0;
}

// not used
void clipwall()
{

}

void BuildNear(int x, int y, int walldist, int nSector)
{
    NearSector[0] = nSector;
    NearCount = 1;

    int i = 0;

    while (i < NearCount)
    {
        short nSector = NearSector[i];

        short nWall = sector[nSector].wallptr;
        short nWallCount = sector[nSector].wallnum;

        while (1)
        {
            nWallCount--;
            if (nWallCount < 0)
            {
                i++;
                break;
            }

            short nNextSector = wall[nWall].nextsector;

            if (nNextSector >= 0)
            {
                int j = 0;
                for (; j < NearCount; j++)
                {
                    // loc_14F4D:
                    if (nNextSector == NearSector[j])
                        break;
                }

                if (j >= NearCount)
                {
                    vec2_t pos = { x, y };
                    if (clipinsidebox(&pos, nWall, walldist))
                    {
                        NearSector[NearCount] = wall[nWall].nextsector;
                        NearCount++;
                    }
                }
            }

            nWall++;
        }
    }
}

int BelowNear(short nSprite)
{
    short nSector = sprite[nSprite].sectnum;
    int z = sprite[nSprite].z;

    int var_24, z2;

    if ((lohit & 0xC000) == 0xC000)
    {
        var_24 = lohit & 0xC000;
        z2 = sprite[lohit & 0x3FFF].z;
    }
    else
    {
        var_24 = 0x20000;
        z2 = sector[nSector].floorz + SectDepth[nSector];

        if (NearCount > 0)
        {
            short edx;

            for (int i = 0; i < NearCount; i++)
            {
                int nSect2 = NearSector[i];

                while (nSect2 >= 0)
                {
                    edx = nSect2;
                    nSect2 = SectBelow[nSect2];
                }

                int ecx = sector[edx].floorz + SectDepth[edx];
                int eax = ecx - z;

                if (eax < 0 && eax >= -5120)
                {
                    z2 = ecx;
                    nSector = edx;
                }
            }
        }
    }

    if (z2 < sprite[nSprite].z)
    {
        sprite[nSprite].z = z2;
        overridesect = nSector;
        sprite[nSprite].zvel = 0;

        bTouchFloor = true;

        return var_24;
    }
    else
    {
        return 0;
    }
}

int movespritez(short nSprite, int z, int height, int, int clipdist)
{
    spritetype* pSprite = &sprite[nSprite];
    short nSector = pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    overridesect = nSector;
    short edi = nSector;

    // backup cstat
    uint16_t cstat = pSprite->cstat;

    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;

    int nRet = 0;

    short nSectFlags = SectFlag[nSector];

    if (nSectFlags & kSectUnderwater) {
        z >>= 1;
    }

    int spriteZ = pSprite->z;
    int floorZ = sector[nSector].floorz;

    int ebp = spriteZ + z;
    int eax = sector[nSector].ceilingz + (height >> 1);

    if ((nSectFlags & kSectUnderwater) && ebp < eax) {
        ebp = eax;
    }

    // loc_151E7:
    while (ebp > sector[pSprite->sectnum].floorz && SectBelow[pSprite->sectnum] >= 0)
    {
        edi = SectBelow[pSprite->sectnum];

        mychangespritesect(nSprite, edi);
    }

    if (edi != nSector)
    {
        pSprite->z = ebp;

        if (SectFlag[edi] & kSectUnderwater)
        {
            if (nSprite == PlayerList[nLocalPlayer].nSprite) {
                D3PlayFX(StaticSound[kSound2], nSprite);
            }

            if (pSprite->statnum <= 107) {
                pSprite->hitag = 0;
            }
        }
    }
    else
    {
        while ((ebp < sector[pSprite->sectnum].ceilingz) && (SectAbove[pSprite->sectnum] >= 0))
        {
            edi = SectAbove[pSprite->sectnum];

            mychangespritesect(nSprite, edi);
        }
    }

    // This function will keep the player from falling off cliffs when you're too close to the edge.
    // This function finds the highest and lowest z coordinates that your clipping BOX can get to.
    getzrange_old(pSprite->x, pSprite->y, pSprite->z - 256, pSprite->sectnum,
        &sprceiling, &hihit, &sprfloor, &lohit, 128, CLIPMASK0);

    int mySprfloor = sprfloor;

    if ((lohit & 0xC000) != 0xC000) {
        mySprfloor += SectDepth[pSprite->sectnum];
    }

    if (ebp > mySprfloor)
    {
        if (z > 0)
        {
            bTouchFloor = true;

            if ((lohit & 0xC000) == 0xC000)
            {
                // Path A
                short nFloorSprite = lohit & 0x3FFF;

                if (pSprite->statnum == 100 && sprite[nFloorSprite].statnum != 0 && sprite[nFloorSprite].statnum < 100)
                {
                    short nDamage = (z >> 9);
                    if (nDamage)
                    {
                        runlist_DamageEnemy(nFloorSprite, nSprite, nDamage << 1);
                    }

                    pSprite->zvel = -z;
                }
                else
                {
                    if (sprite[nFloorSprite].statnum == 0 || sprite[nFloorSprite].statnum > 199)
                    {
                        nRet |= 0x20000;
                    }
                    else
                    {
                        nRet |= lohit;
                    }

                    pSprite->zvel = 0;
                }
            }
            else
            {
                // Path B
                if (SectBelow[pSprite->sectnum] == -1)
                {
                    nRet |= 0x20000;

                    short nSectDamage = SectDamage[pSprite->sectnum];

                    if (nSectDamage != 0)
                    {
                        if (pSprite->hitag < 15)
                        {
                            IgniteSprite(nSprite);
                            pSprite->hitag = 20;
                        }
                        nSectDamage >>= 2;
                        nSectDamage = nSectDamage - (nSectDamage>>2);
                        if (nSectDamage) {
                            runlist_DamageEnemy(nSprite, -1, nSectDamage);
                        }
                    }

                    pSprite->zvel = 0;
                }
            }
        }

        // loc_1543B:
        ebp = mySprfloor;
        pSprite->z = mySprfloor;
    }
    else
    {
        if ((ebp - height) < sprceiling && ((hihit & 0xC000) == 0xC000 || SectAbove[pSprite->sectnum] == -1))
        {
            ebp = sprceiling + height;
            nRet |= 0x10000;
        }
    }

    if (spriteZ <= floorZ && ebp > floorZ)
    {
        if ((SectDepth[nSector] != 0) || (edi != nSector && (SectFlag[edi] & kSectUnderwater)))
        {
            assert(nSector >= 0 && nSector < kMaxSectors);
            BuildSplash(nSprite, nSector);
        }
    }

    pSprite->cstat = cstat; // restore cstat
    pSprite->z = ebp;

    if (pSprite->statnum == 100)
    {
        BuildNear(pSprite->x, pSprite->y, clipdist + (clipdist / 2), pSprite->sectnum);
        nRet |= BelowNear(nSprite);
    }

    return nRet;
}

int GetSpriteHeight(int nSprite)
{
    return tileHeight(sprite[nSprite].picnum) * sprite[nSprite].yrepeat * 4;
}

int movesprite(short nSprite, int dx, int dy, int dz, int, int flordist, unsigned int clipmask)
{
    spritetype *pSprite = &sprite[nSprite];
    bTouchFloor = false;

    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;

    int nSpriteHeight = GetSpriteHeight(nSprite);

    int nClipDist = (int8_t)pSprite->clipdist << 2;

    short nSector = pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    int floorZ = sector[nSector].floorz;

    int nRet = 0;

    if ((SectFlag[nSector] & kSectUnderwater) || (floorZ < z))
    {
        dx >>= 1;
        dy >>= 1;
    }

    nRet |= movespritez(nSprite, dz, nSpriteHeight, flordist, nClipDist);

    nSector = pSprite->sectnum; // modified in movespritez so re-grab this variable

    if (pSprite->statnum == 100)
    {
        short nPlayer = GetPlayerFromSprite(nSprite);

        int varA = 0;
        int varB = 0;

        CheckSectorFloor(overridesect, pSprite->z, &varB, &varA);

        if (varB || varA)
        {
            nXDamage[nPlayer] = varB;
            nYDamage[nPlayer] = varA;
        }

        dx += nXDamage[nPlayer];
        dy += nYDamage[nPlayer];
    }
    else
    {
        CheckSectorFloor(overridesect, pSprite->z, &dx, &dy);
    }

    nRet |= (uint16_t)clipmove_old(&pSprite->x, &pSprite->y, &pSprite->z, &nSector, dx, dy, nClipDist, nSpriteHeight, flordist, clipmask);

    if ((nSector != pSprite->sectnum) && nSector >= 0)
    {
        if (nRet & 0x20000) {
            dz = 0;
        }

        if ((sector[nSector].floorz - z) < (dz + flordist))
        {
            pSprite->x = x;
            pSprite->y = y;
        }
        else
        {
            mychangespritesect(nSprite, nSector);

            if (pSprite->pal < 5 && !pSprite->hitag)
            {
                pSprite->pal = sector[pSprite->sectnum].ceilingpal;
            }
        }
    }

    return nRet;
}

void Gravity(short nSprite)
{
    short nSector = sprite[nSprite].sectnum;

    if (SectFlag[nSector] & kSectUnderwater)
    {
        if (sprite[nSprite].statnum != 100)
        {
            if (sprite[nSprite].zvel <= 1024)
            {
                if (sprite[nSprite].zvel < 2048) {
                    sprite[nSprite].zvel += 512;
                }
            }
            else
            {
                sprite[nSprite].zvel -= 64;
            }
        }
        else
        {
            if (sprite[nSprite].zvel > 0)
            {
                sprite[nSprite].zvel -= 64;
                if (sprite[nSprite].zvel < 0) {
                    sprite[nSprite].zvel = 0;
                }
            }
            else if (sprite[nSprite].zvel < 0)
            {
                sprite[nSprite].zvel += 64;
                if (sprite[nSprite].zvel > 0) {
                    sprite[nSprite].zvel = 0;
                }
            }
        }
    }
    else
    {
        sprite[nSprite].zvel += 512;
        if (sprite[nSprite].zvel > 16384) {
            sprite[nSprite].zvel = 16384;
        }
    }
}

int MoveCreature(short nSprite)
{
    return movesprite(nSprite, sprite[nSprite].xvel << 8, sprite[nSprite].yvel << 8, sprite[nSprite].zvel, 15360, -5120, CLIPMASK0);
}

int MoveCreatureWithCaution(int nSprite)
{
    int x = sprite[nSprite].x;
    int y = sprite[nSprite].y;
    int z = sprite[nSprite].z;
    short nSectorPre = sprite[nSprite].sectnum;

    int ecx = MoveCreature(nSprite);

    short nSector = sprite[nSprite].sectnum;

    if (nSector != nSectorPre)
    {
        int zDiff = sector[nSectorPre].floorz - sector[nSector].floorz;
        if (zDiff < 0) {
            zDiff = -zDiff;
        }

        if (zDiff > 15360 || (SectFlag[nSector] & kSectUnderwater) || (SectBelow[nSector] > -1 && SectFlag[SectBelow[nSector]]) || SectDamage[nSector])
        {
            sprite[nSprite].x = x;
            sprite[nSprite].y = y;
            sprite[nSprite].z = z;

            mychangespritesect(nSprite, nSectorPre);

            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
            sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -2);
            sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -2);
            return 0;
        }
    }

    return ecx;
}

int GetAngleToSprite(int nSprite1, int nSprite2)
{
    if (nSprite1 < 0 || nSprite2 < 0)
        return -1;

    return GetMyAngle(sprite[nSprite2].x - sprite[nSprite1].x, sprite[nSprite2].y - sprite[nSprite1].y);
}

int PlotCourseToSprite(int nSprite1, int nSprite2)
{
    if (nSprite1 < 0 || nSprite2 < 0)
        return -1;

    int x = sprite[nSprite2].x - sprite[nSprite1].x;
    int y = sprite[nSprite2].y - sprite[nSprite1].y;

    sprite[nSprite1].ang = GetMyAngle(x, y);

    uint32_t x2 = abs(x);
    uint32_t y2 = abs(y);

    uint32_t diff = x2 * x2 + y2 * y2;

    if (diff > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        diff = INT_MAX;
    }

    return ksqrt(diff);
}

int FindPlayer(int nSprite, int nDistance)
{
    int var_18 = 0;
    if (nSprite >= 0)
        var_18 = 1;

    if (nSprite < 0)
        nSprite = -nSprite;

    if (nDistance < 0)
        nDistance = 100;

    int x = sprite[nSprite].x;
    int y = sprite[nSprite].y;
    short nSector = sprite[nSprite].sectnum;

    int z = sprite[nSprite].z - GetSpriteHeight(nSprite);

    nDistance <<= 8;

    short nPlayerSprite;
    int i = 0;

    while (1)
    {
        if (i >= nTotalPlayers)
            return -1;

        nPlayerSprite = PlayerList[i].nSprite;

        if ((sprite[nPlayerSprite].cstat & 0x101) && (!(sprite[nPlayerSprite].cstat & 0x8000)))
        {
            int v9 = abs(sprite[nPlayerSprite].x - x);

            if (v9 < nDistance)
            {
                int v10 = abs(sprite[nPlayerSprite].y - y);

                if (v10 < nDistance && cansee(sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z - 7680, sprite[nPlayerSprite].sectnum, x, y, z, nSector))
                {
                    break;
                }
            }
        }

        i++;
    }

    if (var_18) {
        PlotCourseToSprite(nSprite, nPlayerSprite);
    }

    return nPlayerSprite;
}

void CheckSectorFloor(short nSector, int z, int *x, int *y)
{
    short nSpeed = SectSpeed[nSector];

    if (!nSpeed) {
        return;
    }

    short nFlag = SectFlag[nSector];
    short nAng = nFlag & kAngleMask;

    if (z >= sector[nSector].floorz)
    {
        *x += bcos(nAng, 3) * nSpeed;
        *y += bsin(nAng, 3) * nSpeed;
    }
    else if (nFlag & 0x800)
    {
        *x += bcos(nAng, 4) * nSpeed;
        *y += bsin(nAng, 4) * nSpeed;
    }
}

int GetUpAngle(short nSprite1, int nVal, short nSprite2, int ecx)
{
    int x = sprite[nSprite2].x - sprite[nSprite1].x;
    int y = sprite[nSprite2].y - sprite[nSprite1].y;

    int ebx = (sprite[nSprite2].z + ecx) - (sprite[nSprite1].z + nVal);
    int edx = (sprite[nSprite2].z + ecx) - (sprite[nSprite1].z + nVal);

    ebx >>= 4;
    edx >>= 8;

    ebx = -ebx;

    ebx -= edx;

    int nSqrt = lsqrt(x * x + y * y);

    return GetMyAngle(nSqrt, ebx);
}

void InitPushBlocks()
{
    nPushBlocks = 0;
}

int GrabPushBlock()
{
    if (nPushBlocks >= kMaxPushBlocks) {
        return -1;
    }

    return nPushBlocks++;
}

void CreatePushBlock(int nSector)
{
    int nBlock = GrabPushBlock();
    int i;

    int startwall = sector[nSector].wallptr;
    int nWalls = sector[nSector].wallnum;

    int xSum = 0;
    int ySum = 0;

    for (i = 0; i < nWalls; i++)
    {
        xSum += wall[startwall + i].x;
        ySum += wall[startwall + i].y;
    }

    int xAvg = xSum / nWalls;
    int yAvg = ySum / nWalls;

    sBlockInfo[nBlock].x = xAvg;
    sBlockInfo[nBlock].y = yAvg;

    int nSprite = insertsprite(nSector, 0);

    sBlockInfo[nBlock].nSprite = nSprite;

    sprite[nSprite].x = xAvg;
    sprite[nSprite].y = yAvg;
    sprite[nSprite].z = sector[nSector].floorz - 256;
    sprite[nSprite].cstat = 0x8000;

    int var_28 = 0;

    for (i = 0; i < nWalls; i++)
    {
        uint32_t xDiff = abs(xAvg - wall[startwall + i].x);
        uint32_t yDiff = abs(yAvg - wall[startwall + i].y);

        uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

        if (sqrtNum > INT_MAX)
        {
            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
            sqrtNum = INT_MAX;
        }

        int nSqrt = ksqrt(sqrtNum);
        if (nSqrt > var_28) {
            var_28 = nSqrt;
        }
    }

    sBlockInfo[nBlock].field_8 = var_28;

    sprite[nSprite].clipdist = (var_28 & 0xFF) << 2;
    sector[nSector].extra = nBlock;
}

void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel)
{
    int i;

    if (nSector == -1) {
        return;
    }

    int nXVect, nYVect;

    if (nAngle < 0)
    {
        nXVect = *nXVel;
        nYVect = *nYVel;
        nAngle = GetMyAngle(nXVect, nYVect);
    }
    else
    {
        nXVect = bcos(nAngle, 6);
        nYVect = bsin(nAngle, 6);
    }

    short nBlock = sector[nSector].extra;
    short nSectFlag = SectFlag[nSector];

    sectortype *pSector = &sector[nSector];
    int nFloorZ = sector[nSector].floorz;
    int startwall = sector[nSector].wallptr;
    int nWalls = sector[nSector].wallnum;

    walltype *pStartWall = &wall[startwall];
    short nNextSector = wall[startwall].nextsector;

    BlockInfo *pBlockInfo = &sBlockInfo[nBlock];

    int x = sBlockInfo[nBlock].x;
    int x_b = sBlockInfo[nBlock].x;

    int y = sBlockInfo[nBlock].y;
    int y_b = sBlockInfo[nBlock].y;

    short nSectorB = nSector;

    int nZVal;
    int z;

    int bUnderwater = nSectFlag & kSectUnderwater;

    if (nSectFlag & kSectUnderwater)
    {
        nZVal = sector[nSector].ceilingz;
        z = sector[nNextSector].ceilingz + 256;

        sector[nSector].ceilingz = sector[nNextSector].ceilingz;
    }
    else
    {
        nZVal = sector[nSector].floorz;
        z = sector[nNextSector].floorz - 256;

        sector[nSector].floorz = sector[nNextSector].floorz;
    }

    clipmove_old((int32_t*)&x, (int32_t*)&y, (int32_t*)&z, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

    int yvect = y - y_b;
    int xvect = x - x_b;

    if (nSectorB != nNextSector && nSectorB != nSector)
    {
        yvect = 0;
        xvect = 0;
    }
    else
    {
        if (!bUnderwater)
        {
            z = nZVal;
            x = x_b;
            y = y_b;

            clipmove_old((int32_t*)&x, (int32_t*)&y, (int32_t*)&z, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

            int ebx = x;
            int ecx = x_b;
            int edx = y;
            int eax = xvect;
            int esi = y_b;

            if (eax < 0) {
                eax = -eax;
            }

            ebx -= ecx;
            ecx = eax;
            eax = ebx;
            edx -= esi;

            if (eax < 0) {
                eax = -eax;
            }

            if (ecx > eax)
            {
                xvect = ebx;
            }

            eax = yvect;
            if (eax < 0) {
                eax = -eax;
            }

            ebx = eax;
            eax = edx;

            if (eax < 0) {
                eax = -eax;
            }

            if (ebx > eax) {
                yvect = edx;
            }
        }
    }

    // GREEN
    if (yvect || xvect)
    {
        SectIterator it(nSector);
        while ((i = it.NextIndex()) >= 0)
        {
            if (sprite[i].statnum < 99)
            {
                sprite[i].x += xvect;
                sprite[i].y += yvect;
            }
            else
            {
                z = sprite[i].z;

                if ((nSectFlag & kSectUnderwater) || z != nZVal || sprite[i].cstat & 0x8000)
                {
                    x = sprite[i].x;
                    y = sprite[i].y;
                    nSectorB = nSector;

                    clipmove_old((int32_t*)&x, (int32_t*)&y, (int32_t*)&z, &nSectorB, -xvect, -yvect, 4 * sprite[i].clipdist, 0, 0, CLIPMASK0);

                    if (nSectorB >= 0 && nSectorB < kMaxSectors && nSectorB != nSector) {
                        mychangespritesect(i, nSectorB);
                    }
                }
            }
        }

        it.Reset(nNextSector);
        while ((i = it.NextIndex()) >= 0)
        {
            if (sprite[i].statnum >= 99)
            {
                x = sprite[i].x;
                y = sprite[i].y;
                z = sprite[i].z;
                nSectorB = nNextSector;

                clipmove_old((int32_t*)&x, (int32_t*)&y, (int32_t*)&z, &nSectorB,
                    -xvect - (bcos(nAngle) * (4 * sprite[i].clipdist)),
                    -yvect - (bsin(nAngle) * (4 * sprite[i].clipdist)),
                    4 * sprite[i].clipdist, 0, 0, CLIPMASK0);


                if (nSectorB != nNextSector && (nSectorB == nSector || nNextSector == nSector))
                {
                    if (nSectorB != nSector || nFloorZ >= sprite[i].z)
                    {
                        if (nSectorB >= 0 && nSectorB < kMaxSectors) {
                            mychangespritesect(i, nSectorB);
                        }
                    }
                    else
                    {
                        movesprite(i,
                            (xvect << 14) + bcos(nAngle) * sprite[i].clipdist,
                            (yvect << 14) + bsin(nAngle) * sprite[i].clipdist,
                            0, 0, 0, CLIPMASK0);
                    }
                }
            }
        }

        for (int i = 0; i < nWalls; i++)
        {
            dragpoint(startwall, xvect + pStartWall->x, yvect + pStartWall->y, 0);
            pStartWall++;
            startwall++;
        }

        pBlockInfo->x += xvect;
        pBlockInfo->y += yvect;
    }

    // loc_163DD
    xvect <<= 14;
    yvect <<= 14;

    if (!(nSectFlag & kSectUnderwater))
    {
        SectIterator it(nSector);
        while ((i = it.NextIndex()) >= 0)
        {
            if (sprite[i].statnum >= 99 && nZVal == sprite[i].z && !(sprite[i].cstat & 0x8000))
            {
                nSectorB = nSector;
                clipmove_old(&sprite[i].x, &sprite[i].y, &sprite[i].z, &nSectorB, xvect, yvect, 4 * sprite[i].clipdist, 5120, -5120, CLIPMASK0);
            }
        }
    }

    if (nSectFlag & kSectUnderwater) {
        pSector->ceilingz = nZVal;
    }
    else {
        pSector->floorz = nZVal;
    }

    *nXVel = xvect;
    *nYVel = yvect;

    /* 
        Update player position variables, in case the player sprite was moved by a sector,
        Otherwise these can be out of sync when used in sound code (before being updated in PlayerFunc()). 
        Can cause local player sounds to play off-centre.
        TODO: Might need to be done elsewhere too?
    */
    int nPlayerSprite = PlayerList[nLocalPlayer].nSprite;
    initx = sprite[nPlayerSprite].x;
    inity = sprite[nPlayerSprite].y;
    initz = sprite[nPlayerSprite].z;
    inita = sprite[nPlayerSprite].ang;
    initsect = sprite[nPlayerSprite].sectnum;
}

void SetQuake(short nSprite, int nVal)
{
    int x = sprite[nSprite].x;
    int y = sprite[nSprite].y;

    nVal *= 256;

    for (int i = 0; i < nTotalPlayers; i++)
    {
        int nPlayerSprite = PlayerList[i].nSprite;

        uint32_t xDiff = abs((int32_t)((sprite[nPlayerSprite].x - x) >> 8));
        uint32_t yDiff = abs((int32_t)((sprite[nPlayerSprite].y - y) >> 8));

        uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

        if (sqrtNum > INT_MAX)
        {
            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
            sqrtNum = INT_MAX;
        }

        int nSqrt = ksqrt(sqrtNum);

        int eax = nVal;

        if (nSqrt)
        {
            eax = eax / nSqrt;

            if (eax >= 256)
            {
                if (eax > 3840) {
                    eax = 3840;
                }
            }
            else
            {
                eax = 0;
            }
        }

        if (eax > nQuake[i]) {
            nQuake[i] = eax;
        }
    }
}

int AngleChase(int nSprite, int nSprite2, int ebx, int ecx, int push1)
{
    int nClipType = sprite[nSprite].statnum != 107;

    /* bjd - need to handle cliptype to clipmask change that occured in later build engine version */
    if (nClipType == 1) {
        nClipType = CLIPMASK1;
    }
    else {
        nClipType = CLIPMASK0;
    }

    short nAngle;

    if (nSprite2 < 0)
    {
        sprite[nSprite].zvel = 0;
        nAngle = sprite[nSprite].ang;
    }
    else
    {
        int nHeight = tileHeight(sprite[nSprite2].picnum) * sprite[nSprite2].yrepeat * 2;

        int nMyAngle = GetMyAngle(sprite[nSprite2].x - sprite[nSprite].x, sprite[nSprite2].y - sprite[nSprite].y);

        uint32_t xDiff = abs(sprite[nSprite2].x - sprite[nSprite].x);
        uint32_t yDiff = abs(sprite[nSprite2].y - sprite[nSprite].y);

        uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

        if (sqrtNum > INT_MAX)
        {
            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
            sqrtNum = INT_MAX;
        }

        int nSqrt = ksqrt(sqrtNum);

        int var_18 = GetMyAngle(nSqrt, ((sprite[nSprite2].z - nHeight) - sprite[nSprite].z) >> 8);

        int nAngDelta = AngleDelta(sprite[nSprite].ang, nMyAngle, 1024);
        int nAngDelta2 = abs(nAngDelta);

        if (nAngDelta2 > 63)
        {
            nAngDelta2 = abs(nAngDelta >> 6);

            ebx /= nAngDelta2;

            if (ebx < 5) {
                ebx = 5;
            }
        }

        int nAngDeltaC = abs(nAngDelta);

        if (nAngDeltaC > push1)
        {
            if (nAngDelta >= 0)
                nAngDelta = push1;
            else
                nAngDelta = -push1;
        }

        nAngle = (nAngDelta + sprite[nSprite].ang) & kAngleMask;
        int nAngDeltaD = AngleDelta(sprite[nSprite].zvel, var_18, 24);

        sprite[nSprite].zvel = (sprite[nSprite].zvel + nAngDeltaD) & kAngleMask;
    }

    sprite[nSprite].ang = nAngle;

    int eax = abs(bcos(sprite[nSprite].zvel));

    int x = ((bcos(nAngle) * ebx) >> 14) * eax;
    int y = ((bsin(nAngle) * ebx) >> 14) * eax;

    int xshift = x >> 8;
    int yshift = y >> 8;

    uint32_t sqrtNum = xshift * xshift + yshift * yshift;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    int z = bsin(sprite[nSprite].zvel) * ksqrt(sqrtNum);

    return movesprite(nSprite, x >> 2, y >> 2, (z >> 13) + bsin(ecx, -5), 0, 0, nClipType);
}

int GetWallNormal(short nWall)
{
    nWall &= kMaxWalls-1;

    int nWall2 = wall[nWall].point2;

    int nAngle = GetMyAngle(wall[nWall2].x - wall[nWall].x, wall[nWall2].y - wall[nWall].y);
    return (nAngle + 512) & kAngleMask;
}

void WheresMyMouth(int nPlayer, int *x, int *y, int *z, short *sectnum)
{
    int nSprite = PlayerList[nPlayer].nSprite;

    *x = sprite[nSprite].x;
    *y = sprite[nSprite].y;

    int height = GetSpriteHeight(nSprite) / 2;

    *z = sprite[nSprite].z - height;
    *sectnum = sprite[nSprite].sectnum;

    clipmove_old((int32_t*)x, (int32_t*)y, (int32_t*)z, sectnum,
        bcos(sprite[nSprite].ang, 7),
        bsin(sprite[nSprite].ang, 7),
        5120, 1280, 1280, CLIPMASK1);
}

void InitChunks()
{
    nCurChunkNum = 0;
    memset(nChunkSprite,   -1, sizeof(nChunkSprite));
    memset(nBodyGunSprite, -1, sizeof(nBodyGunSprite));
    memset(nBodySprite,    -1, sizeof(nBodySprite));
    nCurBodyNum    = 0;
    nCurBodyGunNum = 0;
    nBodyTotal  = 0;
    nChunkTotal = 0;
}

int GrabBodyGunSprite()
{
    int nSprite = nBodyGunSprite[nCurBodyGunNum];

    if (nSprite == -1)
    {
        nSprite = insertsprite(0, 899);
        nBodyGunSprite[nCurBodyGunNum] = nSprite;

        sprite[nSprite].lotag = -1;
        sprite[nSprite].owner = -1;
    }
    else
    {
        int nAnim = sprite[nSprite].owner;

        if (nAnim != -1) {
            DestroyAnim(nAnim);
        }

        sprite[nSprite].lotag = -1;
        sprite[nSprite].owner = -1;
    }

    nCurBodyGunNum++;
    if (nCurBodyGunNum >= 50) { // TODO - enum/define
        nCurBodyGunNum = 0;
    }

    sprite[nSprite].cstat = 0;

    return nSprite;
}

int GrabBody()
{
    int nSprite;

    do
    {
        nSprite = nBodySprite[nCurBodyNum];

        if (nSprite == -1)
        {
            nSprite = insertsprite(0, 899);
            nBodySprite[nCurBodyNum] = nSprite;
            sprite[nSprite].cstat = 0x8000;
        }

        nCurBodyNum++;
        if (nCurBodyNum >= 50) {
            nCurBodyNum = 0;
        }
    } while (sprite[nSprite].cstat & 0x101);

    if (nBodyTotal < 50) {
        nBodyTotal++;
    }

    sprite[nSprite].cstat = 0;
    return nSprite;
}

int GrabChunkSprite()
{
    int nSprite = nChunkSprite[nCurChunkNum];

    if (nSprite == -1)
    {
        nSprite = insertsprite(0, 899);
        nChunkSprite[nCurChunkNum] = nSprite;
    }
    else if (sprite[nSprite].statnum)
    {
// TODO	MonoOut("too many chunks being used at once!\n");
        return -1;
    }

    changespritestat(nSprite, 899);

    nCurChunkNum++;
    if (nCurChunkNum >= kMaxMoveChunks)
        nCurChunkNum = 0;

    if (nChunkTotal < kMaxMoveChunks)
        nChunkTotal++;

    sprite[nSprite].cstat = 0x80;

    return nSprite;
}

int BuildCreatureChunk(int nVal, int nPic)
{
    int var_14;

    int nSprite = GrabChunkSprite();

    if (nSprite == -1) {
        return -1;
    }

    if (nVal & 0x4000)
    {
        nVal &= 0x3FFF;
        var_14 = 1;
    }
    else
    {
        var_14 = 0;
    }

    nVal &= 0xFFFF;

    sprite[nSprite].x = sprite[nVal].x;
    sprite[nSprite].y = sprite[nVal].y;
    sprite[nSprite].z = sprite[nVal].z;

    mychangespritesect(nSprite, sprite[nVal].sectnum);

    sprite[nSprite].cstat = 0x80;
    sprite[nSprite].shade = -12;
    sprite[nSprite].pal = 0;

    sprite[nSprite].xvel = (RandomSize(5) - 16) << 7;
    sprite[nSprite].yvel = (RandomSize(5) - 16) << 7;
    sprite[nSprite].zvel = (-(RandomSize(8) + 512)) << 3;

    if (var_14)
    {
        sprite[nSprite].xvel *= 4;
        sprite[nSprite].yvel *= 4;
        sprite[nSprite].zvel *= 2;
    }

    sprite[nSprite].xrepeat = 64;
    sprite[nSprite].yrepeat = 64;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = nPic;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].clipdist = 40;

//	GrabTimeSlot(3);

    sprite[nSprite].extra = -1;
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSprite | 0xD0000);
    sprite[nSprite].hitag = runlist_AddRunRec(NewRun, nSprite | 0xD0000);

    return nSprite | 0xD0000;
}

void FuncCreatureChunk(int a, int, int nRun)
{
    int nSprite = RunData[nRun].nVal;
    assert(nSprite >= 0 && nSprite < kMaxSprites);

    int nMessage = a & 0x7F0000;

    if (nMessage != 0x20000)
        return;

    Gravity(nSprite);

    int nSector = sprite[nSprite].sectnum;
    sprite[nSprite].pal = sector[nSector].ceilingpal;

    int nVal = movesprite(nSprite, sprite[nSprite].xvel << 10, sprite[nSprite].yvel << 10, sprite[nSprite].zvel, 2560, -2560, CLIPMASK1);

    if (sprite[nSprite].z >= sector[nSector].floorz)
    {
        // re-grab this variable as it may have changed in movesprite(). Note the check above is against the value *before* movesprite so don't change it.
        nSector = sprite[nSprite].sectnum;

        sprite[nSprite].xvel = 0;
        sprite[nSprite].yvel = 0;
        sprite[nSprite].zvel = 0;
        sprite[nSprite].z = sector[nSector].floorz;
    }
    else
    {
        if (!nVal)
            return;

        short nAngle;

        if (nVal & 0x20000)
        {
            sprite[nSprite].cstat = 0x8000;
        }
        else
        {
            if ((nVal & 0x3C000) == 0x10000)
            {
                sprite[nSprite].xvel >>= 1;
                sprite[nSprite].yvel >>= 1;
                sprite[nSprite].zvel = -sprite[nSprite].zvel;
                return;
            }
            else if ((nVal & 0x3C000) == 0xC000)
            {
                nAngle = sprite[nVal & 0x3FFF].ang;
            }
            else if ((nVal & 0x3C000) == 0x8000)
            {
                nAngle = GetWallNormal(nVal & 0x3FFF);
            }
            else
            {
                return;
            }

            // loc_16E0C
            int nSqrt = lsqrt(((sprite[nSprite].yvel >> 10) * (sprite[nSprite].yvel >> 10)
                + (sprite[nSprite].xvel >> 10) * (sprite[nSprite].xvel >> 10)) >> 8);

            sprite[nSprite].xvel = bcos(nAngle) * (nSqrt >> 1);
            sprite[nSprite].yvel = bsin(nAngle) * (nSqrt >> 1);
            return;
        }
    }

    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_FreeRun(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(sprite[nSprite].hitag);

    changespritestat(nSprite, 0);
    sprite[nSprite].hitag = 0;
    sprite[nSprite].lotag = 0;
}

short UpdateEnemy(short *nEnemy)
{
    if (*nEnemy >= 0)
    {
        if (!(sprite[*nEnemy].cstat & 0x101)) {
            *nEnemy = -1;
        }
    }

    return *nEnemy;
}
END_PS_NS
