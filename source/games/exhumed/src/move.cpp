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

short nPushBlocks;

// TODO - moveme?
short overridesect;

DExhumedActor* nBodySprite[50];

int sprceiling, sprfloor;
Collision loHit, hiHit;

enum
{
	kMaxPushBlocks	= 100,
	kMaxMoveChunks	= 75
};

// think this belongs in init.c?
BlockInfo sBlockInfo[kMaxPushBlocks];

DExhumedActor *nChunkSprite[kMaxMoveChunks];

FSerializer& Serialize(FSerializer& arc, const char* keyname, BlockInfo& w, BlockInfo* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at8", w.field_8)
            ("sprite", w.pActor)
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
        arc ("pushcount", nPushBlocks)
            .Array("blocks", sBlockInfo, nPushBlocks)
            ("chunkcount", nCurChunkNum)
            .Array("chunks", nChunkSprite, kMaxMoveChunks)
            ("overridesect", overridesect)
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

int BelowNear(DExhumedActor* pActor, int x, int y, int walldist, int nSector)
{
    unsigned nearstart = GlobalSectorList.Size();
    GlobalSectorList.Push(nSector);

    unsigned i = nearstart;

    while (i < GlobalSectorList.Size())
    {
        int nSector = GlobalSectorList[i];

        int nWall = sector[nSector].wallptr;
        int nWallCount = sector[nSector].wallnum;

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
                unsigned j = nearstart;
                for (; j < GlobalSectorList.Size(); j++)
                {
                    // loc_14F4D:
                    if (nNextSector == GlobalSectorList[j])
                        break;
                }

                if (j >= GlobalSectorList.Size())
                {
                    vec2_t pos = { x, y };
                    if (clipinsidebox(&pos, nWall, walldist))
                    {
                        GlobalSectorList.Push(wall[nWall].nextsector);
                    }
                }
            }

            nWall++;
        }
    }

    auto pSprite = &pActor->s();
    nSector = pSprite->sectnum;
    int z = pSprite->z;

    int z2;

    if (loHit.type == kHitSprite)
    {
        z2 = loHit.actor->s().z;
    }
    else
    {
        z2 = sector[nSector].floorz + SectDepth[nSector];

        if (GlobalSectorList.Size() > nearstart)
        {
            short edx;

            for (unsigned i = nearstart; i < GlobalSectorList.Size(); i++)
            {
                int nSect2 = GlobalSectorList[i];

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
    GlobalSectorList.Resize(nearstart);

    if (z2 < pSprite->z)
    {
        pSprite->z = z2;
        overridesect = nSector;
        pSprite->zvel = 0;

        bTouchFloor = true;

        return kHitAux2;
    }
    else
    {
        return 0;
    }
}

Collision movespritez(DExhumedActor* pActor, int z, int height, int, int clipdist)
{
    spritetype* pSprite = &pActor->s();
    int nSector =pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    overridesect = nSector;
    short edi = nSector;

    // backup cstat
    uint16_t cstat = pSprite->cstat;

    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;

    Collision nRet(0);

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
    while (ebp > pSprite->sector()->floorz && SectBelow[pSprite->sectnum] >= 0)
    {
        edi = SectBelow[pSprite->sectnum];

        ChangeActorSect(pActor, edi);
    }

    if (edi != nSector)
    {
        pSprite->z = ebp;

        if (SectFlag[edi] & kSectUnderwater)
        {
            if (pActor == PlayerList[nLocalPlayer].Actor()) {
                D3PlayFX(StaticSound[kSound2], pActor);
            }

            if (pSprite->statnum <= 107) {
                pSprite->hitag = 0;
            }
        }
    }
    else
    {
        while ((ebp < pSprite->sector()->ceilingz) && (SectAbove[pSprite->sectnum] >= 0))
        {
            edi = SectAbove[pSprite->sectnum];

            ChangeActorSect(pActor, edi);
        }
    }

    // This function will keep the player from falling off cliffs when you're too close to the edge.
    // This function finds the highest and lowest z coordinates that your clipping BOX can get to.
    int hihit, lohit;
    vec3_t pos = pSprite->pos;
    pos.z -= 256;
    getzrange(&pSprite->pos, pSprite->sectnum,
        &sprceiling, &hihit, &sprfloor, &lohit, 128, CLIPMASK0);
    hiHit.setFromEngine(hihit);
    loHit.setFromEngine(lohit);

    int mySprfloor = sprfloor;

    if (loHit.type != kHitSprite) {
        mySprfloor += SectDepth[pSprite->sectnum];
    }

    if (ebp > mySprfloor)
    {
        if (z > 0)
        {
            bTouchFloor = true;

            if (loHit.type == kHitSprite)
            {
                // Path A
                auto pFloorSprite = &loHit.actor->s();

                if (pSprite->statnum == 100 && pFloorSprite->statnum != 0 && pFloorSprite->statnum < 100)
                {
                    short nDamage = (z >> 9);
                    if (nDamage)
                    {
                        runlist_DamageEnemy(loHit.actor, pActor, nDamage << 1);
                    }

                    pSprite->zvel = -z;
                }
                else
                {
                    if (pFloorSprite->statnum == 0 || pFloorSprite->statnum > 199)
                    {
                        nRet.exbits |= kHitAux2;
                    }
                    else
                    {
                        nRet = loHit;
                    }

                    pSprite->zvel = 0;
                }
            }
            else
            {
                // Path B
                if (SectBelow[pSprite->sectnum] == -1)
                {
                    nRet.exbits |= kHitAux2;

                    short nSectDamage = SectDamage[pSprite->sectnum];

                    if (nSectDamage != 0)
                    {
                        if (pSprite->hitag < 15)
                        {
                            IgniteSprite(pActor);
                            pSprite->hitag = 20;
                        }
                        nSectDamage >>= 2;
                        nSectDamage = nSectDamage - (nSectDamage>>2);
                        if (nSectDamage) {
                            runlist_DamageEnemy(pActor, nullptr, nSectDamage);
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
        if ((ebp - height) < sprceiling && (hiHit.type == kHitSprite || SectAbove[pSprite->sectnum] == -1))
        {
            ebp = sprceiling + height;
            nRet.exbits |= kHitAux1;
        }
    }

    if (spriteZ <= floorZ && ebp > floorZ)
    {
        if ((SectDepth[nSector] != 0) || (edi != nSector && (SectFlag[edi] & kSectUnderwater)))
        {
            assert(nSector >= 0 && nSector < kMaxSectors);
            BuildSplash(pActor, nSector);
        }
    }

    pSprite->cstat = cstat; // restore cstat
    pSprite->z = ebp;

    if (pSprite->statnum == 100)
    {
        nRet.exbits |= BelowNear(pActor, pSprite->x, pSprite->y, clipdist + (clipdist / 2), pSprite->sectnum);
    }

    return nRet;
}

int GetActorHeight(DExhumedActor* actor)
{
    return tileHeight(actor->s().picnum) * actor->s().yrepeat * 4;
}

DExhumedActor* insertActor(int sect, int stat)
{
    int ndx = insertsprite(sect, stat);
    return ndx >= 0 ? &exhumedActors[ndx] : nullptr;
}


Collision movesprite(DExhumedActor* pActor, int dx, int dy, int dz, int ceildist, int flordist, unsigned int clipmask)
{
    spritetype *pSprite = &pActor->s();
    bTouchFloor = false;

    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;

    int nSpriteHeight = GetActorHeight(pActor);

    int nClipDist = (int8_t)pSprite->clipdist << 2;

    int nSector = pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    int floorZ = sector[nSector].floorz;

    if ((SectFlag[nSector] & kSectUnderwater) || (floorZ < z))
    {
        dx >>= 1;
        dy >>= 1;
    }

    Collision nRet = movespritez(pActor, dz, nSpriteHeight, flordist, nClipDist);

    nSector = pSprite->sectnum; // modified in movespritez so re-grab this variable

    if (pSprite->statnum == 100)
    {
        short nPlayer = GetPlayerFromActor(pActor);

        int varA = 0;
        int varB = 0;

        CheckSectorFloor(overridesect, pSprite->z, &varB, &varA);

        if (varB || varA)
        {
            PlayerList[nPlayer].nXDamage = varB;
            PlayerList[nPlayer].nYDamage = varA;
        }

        dx += PlayerList[nPlayer].nXDamage;
        dy += PlayerList[nPlayer].nYDamage;
    }
    else
    {
        CheckSectorFloor(overridesect, pSprite->z, &dx, &dy);
    }

    int colv = clipmove(&pSprite->pos, &nSector, dx, dy, nClipDist, nSpriteHeight, flordist, clipmask);
    Collision coll(colv);
    if (coll.type != kHitNone) // originally this or'ed the two values which can create unpredictable bad values in some edge cases.
    {
        coll.exbits = nRet.exbits;
        nRet = coll;
    }

    if ((nSector != pSprite->sectnum) && nSector >= 0)
    {
        if (nRet.exbits & kHitAux2) {
            dz = 0;
        }

        if ((sector[nSector].floorz - z) < (dz + flordist))
        {
            pSprite->x = x;
            pSprite->y = y;
        }
        else
        {
            ChangeActorSect(pActor, nSector);

            if (pSprite->pal < 5 && !pSprite->hitag)
            {
                pSprite->pal = pSprite->sector()->ceilingpal;
            }
        }
    }

    return nRet;
}

void Gravity(DExhumedActor* actor)
{
    auto pSprite = &actor->s();
    int nSector =pSprite->sectnum;

    if (SectFlag[nSector] & kSectUnderwater)
    {
        if (pSprite->statnum != 100)
        {
            if (pSprite->zvel <= 1024)
            {
                if (pSprite->zvel < 2048) {
                    pSprite->zvel += 512;
                }
            }
            else
            {
                pSprite->zvel -= 64;
            }
        }
        else
        {
            if (pSprite->zvel > 0)
            {
                pSprite->zvel -= 64;
                if (pSprite->zvel < 0) {
                    pSprite->zvel = 0;
                }
            }
            else if (pSprite->zvel < 0)
            {
                pSprite->zvel += 64;
                if (pSprite->zvel > 0) {
                    pSprite->zvel = 0;
                }
            }
        }
    }
    else
    {
        pSprite->zvel += 512;
        if (pSprite->zvel > 16384) {
            pSprite->zvel = 16384;
        }
    }
}

Collision MoveCreature(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
    return movesprite(pActor, pSprite->xvel << 8, pSprite->yvel << 8, pSprite->zvel, 15360, -5120, CLIPMASK0);
}

Collision MoveCreatureWithCaution(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    short nSectorPre = pSprite->sectnum;

    auto ecx = MoveCreature(pActor);

    int nSector =pSprite->sectnum;

    if (nSector != nSectorPre)
    {
        int zDiff = sector[nSectorPre].floorz - sector[nSector].floorz;
        if (zDiff < 0) {
            zDiff = -zDiff;
        }

        if (zDiff > 15360 || (SectFlag[nSector] & kSectUnderwater) || (SectBelow[nSector] > -1 && SectFlag[SectBelow[nSector]]) || SectDamage[nSector])
        {
            pSprite->x = x;
            pSprite->y = y;
            pSprite->z = z;

            ChangeActorSect(pActor, nSectorPre);

            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -2);
            pSprite->yvel = bsin(pSprite->ang, -2);
            return Collision(0);
        }
    }

    return ecx;
}

int GetAngleToSprite(DExhumedActor* a1, DExhumedActor* a2)
{
    if (!a1 || !a2)
        return -1;

	auto pSprite1 = &a1->s();
	auto pSprite2 = &a2->s();

    return GetMyAngle(pSprite2->x - pSprite1->x, pSprite2->y - pSprite1->y);
}

int PlotCourseToSprite(DExhumedActor* pActor1, DExhumedActor* pActor2)
{
    if (pActor1 == nullptr || pActor2 == nullptr)
        return -1;

	auto pSprite1 = &pActor1->s();
	auto pSprite2 = &pActor2->s();
    int x = pSprite2->x - pSprite1->x;
    int y = pSprite2->y - pSprite1->y;

    pSprite1->ang = GetMyAngle(x, y);

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

DExhumedActor* FindPlayer(DExhumedActor* pActor, int nDistance, bool dontengage)
{
    auto pSprite = &pActor->s();
    int var_18 = !dontengage;

    if (nDistance < 0)
        nDistance = 100;

    int x = pSprite->x;
    int y = pSprite->y;
    int nSector =pSprite->sectnum;

    int z = pSprite->z - GetActorHeight(pActor);

    nDistance <<= 8;

    DExhumedActor* pPlayerActor = nullptr;
    int i = 0;

    while (1)
    {
        if (i >= nTotalPlayers)
            return nullptr;

        pPlayerActor = PlayerList[i].Actor();
        auto pPlayerSprite = &pPlayerActor->s();

        if ((pPlayerSprite->cstat & 0x101) && (!(pPlayerSprite->cstat & 0x8000)))
        {
            int v9 = abs(pPlayerSprite->x - x);

            if (v9 < nDistance)
            {
                int v10 = abs(pPlayerSprite->y - y);

                if (v10 < nDistance && cansee(pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z - 7680, pPlayerSprite->sectnum, x, y, z, nSector))
                {
                    break;
                }
            }
        }

        i++;
    }

    if (var_18) {
        PlotCourseToSprite(pActor, pPlayerActor);
    }

    return pPlayerActor;
}

void CheckSectorFloor(int nSector, int z, int *x, int *y)
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

int GetUpAngle(DExhumedActor* pActor1, int nVal, DExhumedActor* pActor2, int ecx)
{
	auto pSprite1 = &pActor1->s();
	auto pSprite2 = &pActor2->s();
    int x = pSprite2->x - pSprite1->x;
    int y = pSprite2->y - pSprite1->y;

    int ebx = (pSprite2->z + ecx) - (pSprite1->z + nVal);
    int edx = (pSprite2->z + ecx) - (pSprite1->z + nVal);

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
    auto sectp = &sector[nSector];
    int nBlock = GrabPushBlock();
    int i;

    int startwall = sectp->wallptr;
    int nWalls = sectp->wallnum;

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

    auto pActor = insertActor(nSector, 0);
    auto pSprite = &pActor->s();

    sBlockInfo[nBlock].pActor = pActor;

    pSprite->x = xAvg;
    pSprite->y = yAvg;
    pSprite->z = sectp->floorz - 256;
    pSprite->cstat = 0x8000;

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

    pSprite->clipdist = (var_28 & 0xFF) << 2;
    sectp->extra = nBlock;
}

void MoveSector(int nSector, int nAngle, int *nXVel, int *nYVel)
{
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
    sectortype *pSector = &sector[nSector];
 

    short nBlock = pSector->extra;
    short nSectFlag = SectFlag[nSector];

    int nFloorZ = pSector->floorz;
    int startwall = pSector->wallptr;
    int nWalls = pSector->wallnum;

    walltype *pStartWall = &wall[startwall];
    short nNextSector = wall[startwall].nextsector;

    BlockInfo *pBlockInfo = &sBlockInfo[nBlock];

    vec3_t pos;

    pos.x = sBlockInfo[nBlock].x;
    int x_b = sBlockInfo[nBlock].x;

    pos.y = sBlockInfo[nBlock].y;
    int y_b = sBlockInfo[nBlock].y;

    int nSectorB = nSector;

    int nZVal;

    int bUnderwater = nSectFlag & kSectUnderwater;

    if (nSectFlag & kSectUnderwater)
    {
        nZVal = pSector->ceilingz;
        pos.z = sector[nNextSector].ceilingz + 256;

        pSector->ceilingz = sector[nNextSector].ceilingz;
    }
    else
    {
        nZVal = pSector->floorz;
        pos.z = sector[nNextSector].floorz - 256;

        pSector->floorz = sector[nNextSector].floorz;
    }

    clipmove(&pos, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

    int yvect = pos.y - y_b;
    int xvect = pos.x - x_b;

    if (nSectorB != nNextSector && nSectorB != nSector)
    {
        yvect = 0;
        xvect = 0;
    }
    else
    {
        if (!bUnderwater)
        {
            pos = { x_b, y_b, nZVal };

            clipmove(&pos, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

            int ebx = pos.x;
            int ecx = x_b;
            int edx = pos.y;
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
        ExhumedSectIterator it(nSector);
        while (auto pActor = it.Next())
        {
            auto sp = &pActor->s();
            if (sp->statnum < 99)
            {
                sp->x += xvect;
                sp->y += yvect;
            }
            else
            {
                pos.z = sp->z;

                if ((nSectFlag & kSectUnderwater) || pos.z != nZVal || sp->cstat & 0x8000)
                {
                    pos.x = sp->x;
                    pos.y = sp->y;
                    nSectorB = nSector;

                    clipmove(&pos, &nSectorB, -xvect, -yvect, 4 * sp->clipdist, 0, 0, CLIPMASK0);

                    if (nSectorB >= 0 && nSectorB < kMaxSectors && nSectorB != nSector) {
                        ChangeActorSect(pActor, nSectorB);
                    }
                }
            }
        }

        it.Reset(nNextSector);
        while (auto pActor = it.Next())
        {
            auto pSprite = &pActor->s();
            if (pSprite->statnum >= 99)
            {
                pos = pSprite->pos;
                nSectorB = nNextSector;

                clipmove(&pos, &nSectorB,
                    -xvect - (bcos(nAngle) * (4 * pSprite->clipdist)),
                    -yvect - (bsin(nAngle) * (4 * pSprite->clipdist)),
                    4 * pSprite->clipdist, 0, 0, CLIPMASK0);


                if (nSectorB != nNextSector && (nSectorB == nSector || nNextSector == nSector))
                {
                    if (nSectorB != nSector || nFloorZ >= pSprite->z)
                    {
                        if (nSectorB >= 0 && nSectorB < kMaxSectors) {
                            ChangeActorSect(pActor, nSectorB);
                        }
                    }
                    else
                    {
                        movesprite(pActor,
                            (xvect << 14) + bcos(nAngle) * pSprite->clipdist,
                            (yvect << 14) + bsin(nAngle) * pSprite->clipdist,
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
        ExhumedSectIterator it(nSector);
        while (auto pActor = it.Next())
        {
            auto pSprite = &pActor->s();
            if (pSprite->statnum >= 99 && nZVal == pSprite->z && !(pSprite->cstat & 0x8000))
            {
                nSectorB = nSector;
                clipmove(&pSprite->pos, &nSectorB, xvect, yvect, 4 * pSprite->clipdist, 5120, -5120, CLIPMASK0);
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
    auto pActor = PlayerList[nLocalPlayer].Actor();
    auto pSprite = &pActor->s();
    initx = pSprite->x;
    inity = pSprite->y;
    initz = pSprite->z;
    inita = pSprite->ang;
    initsect = pSprite->sectnum;
}

void SetQuake(DExhumedActor* pActor, int nVal)
{
    auto pSprite = &pActor->s();
    int x = pSprite->x;
    int y = pSprite->y;

    nVal *= 256;

    for (int i = 0; i < nTotalPlayers; i++)
    {
        auto pPlayerActor = PlayerList[i].Actor();


        uint32_t xDiff = abs((int32_t)((pPlayerActor->s().x - x) >> 8));
        uint32_t yDiff = abs((int32_t)((pPlayerActor->s().y - y) >> 8));

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

Collision AngleChase(DExhumedActor* pActor, DExhumedActor* pActor2, int ebx, int ecx, int push1) 
{
    auto pSprite = &pActor->s();
    int nClipType = pSprite->statnum != 107;

    /* bjd - need to handle cliptype to clipmask change that occured in later build engine version */
    if (nClipType == 1) {
        nClipType = CLIPMASK1;
    }
    else {
        nClipType = CLIPMASK0;
    }

    short nAngle;

    if (pActor2 == nullptr)
    {
        pSprite->zvel = 0;
        nAngle = pSprite->ang;
    }
    else
    {
		auto pSprite2 = &pActor2->s();

        int nHeight = tileHeight(pSprite2->picnum) * pSprite2->yrepeat * 2;

        int nMyAngle = GetMyAngle(pSprite2->x - pSprite->x, pSprite2->y - pSprite->y);

        uint32_t xDiff = abs(pSprite2->x - pSprite->x);
        uint32_t yDiff = abs(pSprite2->y - pSprite->y);

        uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

        if (sqrtNum > INT_MAX)
        {
            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
            sqrtNum = INT_MAX;
        }

        int nSqrt = ksqrt(sqrtNum);

        int var_18 = GetMyAngle(nSqrt, ((pSprite2->z - nHeight) - pSprite->z) >> 8);

        int nAngDelta = AngleDelta(pSprite->ang, nMyAngle, 1024);
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

        nAngle = (nAngDelta + pSprite->ang) & kAngleMask;
        int nAngDeltaD = AngleDelta(pSprite->zvel, var_18, 24);

        pSprite->zvel = (pSprite->zvel + nAngDeltaD) & kAngleMask;
    }

    pSprite->ang = nAngle;

    int eax = abs(bcos(pSprite->zvel));

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

    int z = bsin(pSprite->zvel) * ksqrt(sqrtNum);

    return movesprite(pActor, x >> 2, y >> 2, (z >> 13) + bsin(ecx, -5), 0, 0, nClipType);
}

int GetWallNormal(int nWall)
{
    nWall &= kMaxWalls-1;

    int nWall2 = wall[nWall].point2;

    int nAngle = GetMyAngle(wall[nWall2].x - wall[nWall].x, wall[nWall2].y - wall[nWall].y);
    return (nAngle + 512) & kAngleMask;
}

void WheresMyMouth(int nPlayer, vec3_t* pos, int *sectnum)
{
    auto pActor = PlayerList[nPlayer].Actor();
	auto pSprite = &pActor->s();

    int height = GetActorHeight(pActor) >> 1;

    *sectnum = pSprite->sectnum;
    *pos = pSprite->pos;
    pos->z -= height;

    clipmove(pos, sectnum,
        bcos(pSprite->ang, 7),
        bsin(pSprite->ang, 7),
        5120, 1280, 1280, CLIPMASK1);
}

void InitChunks()
{
    nCurChunkNum = 0;
    memset(nChunkSprite,   0, sizeof(nChunkSprite));
    memset(nBodyGunSprite, 0, sizeof(nBodyGunSprite));
    memset(nBodySprite,    0, sizeof(nBodySprite));
    nCurBodyNum    = 0;
    nCurBodyGunNum = 0;
    nBodyTotal  = 0;
    nChunkTotal = 0;
}

DExhumedActor* GrabBodyGunSprite()
{
    auto pActor = nBodyGunSprite[nCurBodyGunNum];
	spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(0, 899);
		pSprite = &pActor->s();
        nBodyGunSprite[nCurBodyGunNum] = pActor;

        pSprite->lotag = -1;
        pSprite->owner = -1;
    }
    else
    {
		pSprite = &pActor->s();
        DestroyAnim(pActor);

        pSprite->lotag = -1;
        pSprite->owner = -1;
    }

    nCurBodyGunNum++;
    if (nCurBodyGunNum >= 50) { // TODO - enum/define
        nCurBodyGunNum = 0;
    }

    pSprite->cstat = 0;

    return pActor;
}

DExhumedActor* GrabBody()
{
	DExhumedActor* pActor = nullptr;
    spritetype* pSprite = nullptr;
    do
    {
        pActor = nBodySprite[nCurBodyNum];

        if (pActor == nullptr)
        {
            pActor = insertActor(0, 899);
            pSprite = &pActor->s();
            nBodySprite[nCurBodyNum] = pActor;
            pSprite->cstat = 0x8000;
        }
		else
			pSprite = &pActor->s();


        nCurBodyNum++;
        if (nCurBodyNum >= 50) {
            nCurBodyNum = 0;
        }
    } while (pSprite->cstat & 0x101);

    if (nBodyTotal < 50) {
        nBodyTotal++;
    }

    pSprite->cstat = 0;
    return pActor;
}

DExhumedActor* GrabChunkSprite()
{
    auto pActor = nChunkSprite[nCurChunkNum];

    if (pActor == nullptr)
    {
        pActor = insertActor(0, 899);
		nChunkSprite[nCurChunkNum] = pActor;
    }
    else if (pActor->s().statnum)
    {
// TODO	MonoOut("too many chunks being used at once!\n");
        return nullptr;
    }

    ChangeActorStat(pActor, 899);

    nCurChunkNum++;
    if (nCurChunkNum >= kMaxMoveChunks)
        nCurChunkNum = 0;

    if (nChunkTotal < kMaxMoveChunks)
        nChunkTotal++;

    pActor->s().cstat = 0x80;

    return pActor;
}

DExhumedActor* BuildCreatureChunk(DExhumedActor* pSrc, int nPic, bool bSpecial)
{
    auto actor = GrabChunkSprite();

    if (actor == nullptr) {
        return nullptr;
    }
	auto pSprite = &actor->s();
    auto pSrcSpr = &pSrc->s();

    pSprite->x = pSrcSpr->x;
    pSprite->y = pSrcSpr->y;
    pSprite->z = pSrcSpr->z;

    ChangeActorSect(actor, pSrcSpr->sectnum);

    pSprite->cstat = 0x80;
    pSprite->shade = -12;
    pSprite->pal = 0;

    pSprite->xvel = (RandomSize(5) - 16) << 7;
    pSprite->yvel = (RandomSize(5) - 16) << 7;
    pSprite->zvel = (-(RandomSize(8) + 512)) << 3;

    if (bSpecial)
    {
        pSprite->xvel *= 4;
        pSprite->yvel *= 4;
        pSprite->zvel *= 2;
    }

    pSprite->xrepeat = 64;
    pSprite->yrepeat = 64;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = nPic;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->clipdist = 40;

//	GrabTimeSlot(3);

    pSprite->extra = -1;
    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, actor, 0xD0000);
    pSprite->hitag = runlist_AddRunRec(NewRun, actor, 0xD0000);

    return actor;
}

void AICreatureChunk::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();

    Gravity(pActor);

    int nSector = pSprite->sectnum;
    pSprite->pal = sector[nSector].ceilingpal;

    auto nVal = movesprite(pActor, pSprite->xvel << 10, pSprite->yvel << 10, pSprite->zvel, 2560, -2560, CLIPMASK1);

    if (pSprite->z >= sector[nSector].floorz)
    {
        // re-grab this variable as it may have changed in movesprite(). Note the check above is against the value *before* movesprite so don't change it.
        nSector = pSprite->sectnum;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->z = sector[nSector].floorz;
    }
    else
    {
        if (!nVal.type && !nVal.exbits)
            return;

        short nAngle;

        if (nVal.exbits & kHitAux2)
        {
            pSprite->cstat = 0x8000;
        }
        else
        {
            if (nVal.exbits & kHitAux1)
            {
                pSprite->xvel >>= 1;
                pSprite->yvel >>= 1;
                pSprite->zvel = -pSprite->zvel;
                return;
            }
            else if (nVal.type == kHitSprite)
            {
                nAngle = nVal.actor->s().ang;
            }
            else if (nVal.type == kHitWall)
            {
                nAngle = GetWallNormal(nVal.index);
            }
            else
            {
                return;
            }

            // loc_16E0C
            int nSqrt = lsqrt(((pSprite->yvel >> 10) * (pSprite->yvel >> 10)
                + (pSprite->xvel >> 10) * (pSprite->xvel >> 10)) >> 8);

            pSprite->xvel = bcos(nAngle) * (nSqrt >> 1);
            pSprite->yvel = bsin(nAngle) * (nSqrt >> 1);
            return;
        }
    }

    runlist_DoSubRunRec(pSprite->owner);
    runlist_FreeRun(pSprite->lotag - 1);
    runlist_SubRunRec(pSprite->hitag);

    ChangeActorStat(pActor, 0);
    pSprite->hitag = 0;
    pSprite->lotag = 0;
}

DExhumedActor* UpdateEnemy(DExhumedActor** ppEnemy)
{
    if (*ppEnemy)
    {
        if (!((*ppEnemy)->s().cstat & 0x101)) {
            *ppEnemy = nullptr;
        }
    }

    return *ppEnemy;
}
END_PS_NS
