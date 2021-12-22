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

int nPushBlocks;

// TODO - moveme?
sectortype* overridesect;

enum
{
    kMaxPushBlocks = 100,
    kMaxMoveChunks = 75
};


TObjPtr<DExhumedActor*> nBodySprite[50];
TObjPtr<DExhumedActor*> nChunkSprite[kMaxMoveChunks];
BlockInfo sBlockInfo[kMaxPushBlocks];
TObjPtr<DExhumedActor*> nBodyGunSprite[50];
int nCurBodyGunNum;

int sprceiling, sprfloor;
Collision loHit, hiHit;

// think this belongs in init.c?


size_t MarkMove()
{
    GC::MarkArray(nBodySprite, 50);
    GC::MarkArray(nChunkSprite, kMaxMoveChunks);
    for(int i = 0; i < nPushBlocks; i++)
        GC::Mark(sBlockInfo[i].pActor);
    return 50 + kMaxMoveChunks + nPushBlocks;
}

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
            .Array("bodysprite", nBodySprite, countof(nBodySprite))
            ("curbodygun", nCurBodyGunNum)
            .Array("bodygunsprite", nBodyGunSprite, countof(nBodyGunSprite))
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

int BelowNear(DExhumedActor* pActor, int x, int y, int walldist)
{
    auto pSprite = &pActor->s();
    auto pSector = pSprite->sector();
    int z = pSprite->z;

    int z2;

    if (loHit.type == kHitSprite)
    {
        z2 = loHit.actor()->spr.z;
    }
    else
    {
        z2 = pSector->floorz + pSector->Depth;

        BFSSectorSearch search(pSector);

        sectortype* pTempSect = nullptr;
        while (auto pCurSector = search.GetNext())
        {
            for (auto& wal : wallsofsector(pCurSector))
            {
                if (wal.twoSided())
                {
                    if (!search.Check(wal.nextSector()))
                    {
                        vec2_t pos = { x, y };
                        if (clipinsidebox(&pos, wallnum(&wal), walldist))
                        {
                            search.Add(wal.nextSector());
                        }
                    }
                }
            }

            auto pSect2 = pCurSector;

            while (pSect2)
            {
                pTempSect = pSect2;
                pSect2 = pSect2->pBelow;
            }

            int ecx = pTempSect->floorz + pTempSect->Depth;
            int eax = ecx - z;

            if (eax < 0 && eax >= -5120)
            {
                z2 = ecx;
                pSector = pTempSect;
            }
        }
    }
    

    if (z2 < pSprite->z)
    {
        pSprite->z = z2;
        overridesect = pSector;
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
    auto pSector = pSprite->sector();
    assert(pSector);

    overridesect = pSector;
    auto pSect2 = pSector;

    // backup cstat
    auto cstat = pSprite->cstat;

    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;

    Collision nRet;
    nRet.setNone();

    int nSectFlags = pSector->Flag;

    if (nSectFlags & kSectUnderwater) {
        z >>= 1;
    }

    int spriteZ = pSprite->z;
    int floorZ = pSector->floorz;

    int ebp = spriteZ + z;
    int eax = pSector->ceilingz + (height >> 1);

    if ((nSectFlags & kSectUnderwater) && ebp < eax) {
        ebp = eax;
    }

    // loc_151E7:
    while (ebp > pSprite->sector()->floorz && pSprite->sector()->pBelow != nullptr)
    {
        ChangeActorSect(pActor, pSprite->sector()->pBelow);
    }

    if (pSect2 != pSector)
    {
        pSprite->z = ebp;

        if (pSect2->Flag & kSectUnderwater)
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
        while ((ebp < pSprite->sector()->ceilingz) && (pSprite->sector()->pAbove != nullptr))
        {
            ChangeActorSect(pActor, pSprite->sector()->pAbove);
        }
    }

    // This function will keep the player from falling off cliffs when you're too close to the edge.
    // This function finds the highest and lowest z coordinates that your clipping BOX can get to.
    vec3_t pos = pSprite->pos;
    pos.z -= 256;
    getzrange(pos, pSprite->sector(), &sprceiling, hiHit, &sprfloor, loHit, 128, CLIPMASK0);

    int mySprfloor = sprfloor;

    if (loHit.type != kHitSprite) {
        mySprfloor += pSprite->sector()->Depth;
    }

    if (ebp > mySprfloor)
    {
        if (z > 0)
        {
            bTouchFloor = true;

            if (loHit.type == kHitSprite)
            {
                // Path A
                auto pFloorSprite = &loHit.actor()->s();

                if (pSprite->statnum == 100 && pFloorSprite->statnum != 0 && pFloorSprite->statnum < 100)
                {
                    int nDamage = (z >> 9);
                    if (nDamage)
                    {
                        runlist_DamageEnemy(loHit.actor(), pActor, nDamage << 1);
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
                if (pSprite->sector()->pBelow == nullptr)
                {
                    nRet.exbits |= kHitAux2;

                    int nSectDamage = pSprite->sector()->Damage;

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
        if ((ebp - height) < sprceiling && (hiHit.type == kHitSprite || pSprite->sector()->pAbove == nullptr))
        {
            ebp = sprceiling + height;
            nRet.exbits |= kHitAux1;
        }
    }

    if (spriteZ <= floorZ && ebp > floorZ)
    {
        if ((pSector->Depth != 0) || (pSect2 != pSector && (pSect2->Flag & kSectUnderwater)))
        {
            BuildSplash(pActor, pSector);
        }
    }

    pSprite->cstat = cstat; // restore cstat
    pSprite->z = ebp;

    if (pSprite->statnum == 100)
    {
        nRet.exbits |= BelowNear(pActor, pSprite->x, pSprite->y, clipdist + (clipdist / 2));
    }

    return nRet;
}

int GetActorHeight(DExhumedActor* actor)
{
    return tileHeight(actor->spr.picnum) * actor->spr.yrepeat * 4;
}

DExhumedActor* insertActor(sectortype* s, int st)
{
    return static_cast<DExhumedActor*>(::InsertActor(RUNTIME_CLASS(DExhumedActor), s, st));
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

    auto pSector = pSprite->sector();
    assert(pSector);

    int floorZ = pSector->floorz;

    if ((pSector->Flag & kSectUnderwater) || (floorZ < z))
    {
        dx >>= 1;
        dy >>= 1;
    }

    Collision nRet = movespritez(pActor, dz, nSpriteHeight, flordist, nClipDist);

    pSector = pSprite->sector(); // modified in movespritez so re-grab this variable

    if (pSprite->statnum == 100)
    {
        int nPlayer = GetPlayerFromActor(pActor);

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

    Collision coll;
    clipmove(pSprite->pos, &pSector, dx, dy, nClipDist, nSpriteHeight, flordist, clipmask, coll);
    if (coll.type != kHitNone) // originally this or'ed the two values which can create unpredictable bad values in some edge cases.
    {
        coll.exbits = nRet.exbits;
        nRet = coll;
    }

    if ((pSector != pSprite->sector()) && pSector != nullptr)
    {
        if (nRet.exbits & kHitAux2) {
            dz = 0;
        }

        if ((pSector->floorz - z) < (dz + flordist))
        {
            pSprite->x = x;
            pSprite->y = y;
        }
        else
        {
            ChangeActorSect(pActor, pSector);

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

    if (pSprite->sector()->Flag & kSectUnderwater)
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
    auto pSectorPre = pSprite->sector();

    auto ecx = MoveCreature(pActor);

    auto pSector =pSprite->sector();

    if (pSector != pSectorPre)
    {
        int zDiff = pSectorPre->floorz - pSector->floorz;
        if (zDiff < 0) {
            zDiff = -zDiff;
        }

        if (zDiff > 15360 || (pSector->Flag & kSectUnderwater) || (pSector->pBelow != nullptr && pSector->pBelow->Flag) || pSector->Damage)
        {
            pSprite->x = x;
            pSprite->y = y;
            pSprite->z = z;

            ChangeActorSect(pActor, pSectorPre);

            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -2);
            pSprite->yvel = bsin(pSprite->ang, -2);
            Collision c;
            c.setNone();
            return c;
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
    auto pSector =pSprite->sector();

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

        if ((pPlayerSprite->cstat & CSTAT_SPRITE_BLOCK_ALL) && (!(pPlayerSprite->cstat & CSTAT_SPRITE_INVISIBLE)))
        {
            int v9 = abs(pPlayerSprite->x - x);

            if (v9 < nDistance)
            {
                int v10 = abs(pPlayerSprite->y - y);

                if (v10 < nDistance && cansee(pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z - 7680, pPlayerSprite->sector(), x, y, z, pSector))
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

void CheckSectorFloor(sectortype* pSector, int z, int *x, int *y)
{
    int nSpeed = pSector->Speed;

    if (!nSpeed) {
        return;
    }

    int nFlag = pSector->Flag;
    int nAng = nFlag & kAngleMask;

    if (z >= pSector->floorz)
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
    memset(sBlockInfo, 0, sizeof(sBlockInfo));
}

int GrabPushBlock()
{
    if (nPushBlocks >= kMaxPushBlocks) {
        return -1;
    }

    return nPushBlocks++;
}

void CreatePushBlock(sectortype* pSector)
{
    int nBlock = GrabPushBlock();

    int xSum = 0;
    int ySum = 0;

    for (auto& wal : wallsofsector(pSector))
    {
        xSum += wal.x;
        ySum += wal.y;
    }

    int xAvg = xSum / pSector->wallnum;
    int yAvg = ySum / pSector->wallnum;

    sBlockInfo[nBlock].x = xAvg;
    sBlockInfo[nBlock].y = yAvg;

    auto pActor = insertActor(pSector, 0);
    auto pSprite = &pActor->s();

    sBlockInfo[nBlock].pActor = pActor;

    pSprite->x = xAvg;
    pSprite->y = yAvg;
    pSprite->z = pSector->floorz - 256;
    pSprite->cstat = CSTAT_SPRITE_INVISIBLE;

    int var_28 = 0;

	for (auto& wal : wallsofsector(pSector))
    {
        uint32_t xDiff = abs(xAvg - wal.x);
        uint32_t yDiff = abs(yAvg - wal.y);

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
    pSector->extra = nBlock;
}

void MoveSector(sectortype* pSector, int nAngle, int *nXVel, int *nYVel)
{
    if (pSector == nullptr) {
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

    int nBlock = pSector->extra;
    int nSectFlag = pSector->Flag;

    int nFloorZ = pSector->floorz;

    walltype *pStartWall = pSector->firstWall();
    sectortype* pNextSector = pStartWall->nextSector();

    BlockInfo *pBlockInfo = &sBlockInfo[nBlock];

    vec3_t pos;

    pos.x = sBlockInfo[nBlock].x;
    int x_b = sBlockInfo[nBlock].x;

    pos.y = sBlockInfo[nBlock].y;
    int y_b = sBlockInfo[nBlock].y;


    int nZVal;

    int bUnderwater = nSectFlag & kSectUnderwater;

    if (nSectFlag & kSectUnderwater)
    {
        nZVal = pSector->ceilingz;
        pos.z = pNextSector->ceilingz + 256;

        pSector->ceilingz = pNextSector->ceilingz;
    }
    else
    {
        nZVal = pSector->floorz;
        pos.z = pNextSector->floorz - 256;

        pSector->floorz = pNextSector->floorz;
    }

    auto pSectorB = pSector;
    Collision scratch;
    clipmove(pos, &pSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1, scratch);

    int yvect = pos.y - y_b;
    int xvect = pos.x - x_b;

    if (pSectorB != pNextSector && pSectorB != pSector)
    {
        yvect = 0;
        xvect = 0;
    }
    else
    {
        if (!bUnderwater)
        {
            pos = { x_b, y_b, nZVal };

            Collision scratch;
            clipmove(pos, &pSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1, scratch);

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
        ExhumedSectIterator it(pSector);
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

                if ((nSectFlag & kSectUnderwater) || pos.z != nZVal || sp->cstat & CSTAT_SPRITE_INVISIBLE)
                {
                    pos.x = sp->x;
                    pos.y = sp->y;
                    pSectorB = pSector;

                    Collision scratch;
                    clipmove(pos, &pSectorB, -xvect, -yvect, 4 * sp->clipdist, 0, 0, CLIPMASK0, scratch);

                    if (pSectorB) {
                        ChangeActorSect(pActor, pSectorB);
                    }
                }
            }
        }
        it.Reset(pNextSector);
        while (auto pActor = it.Next())
        {
            auto pSprite = &pActor->s();
            if (pSprite->statnum >= 99)
            {
                pos = pSprite->pos;
                pSectorB = pNextSector;

                Collision scratch;
                clipmove(pos, &pSectorB,
                    -xvect - (bcos(nAngle) * (4 * pSprite->clipdist)),
                    -yvect - (bsin(nAngle) * (4 * pSprite->clipdist)),
                    4 * pSprite->clipdist, 0, 0, CLIPMASK0, scratch);


                if (pSectorB != pNextSector && (pSectorB == pSector || pNextSector == pSector))
                {
                    if (pSectorB != pSector || nFloorZ >= pSprite->z)
                    {
                        if (pSectorB) {
                            ChangeActorSect(pActor, pSectorB);
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

		for(auto& wal : wallsofsector(pSector))
        {
            dragpoint(&wal, xvect + wal.x, yvect + wal.y);
        }

        pBlockInfo->x += xvect;
        pBlockInfo->y += yvect;
    }

    // loc_163DD
    xvect <<= 14;
    yvect <<= 14;

    if (!(nSectFlag & kSectUnderwater))
    {
        ExhumedSectIterator it(pSector);
        while (auto pActor = it.Next())
        {
            auto pSprite = &pActor->s();
            if (pSprite->statnum >= 99 && nZVal == pSprite->z && !(pSprite->cstat & CSTAT_SPRITE_INVISIBLE))
            {
                pSectorB = pSector;
                Collision scratch;
                clipmove(pSprite->pos, &pSectorB, xvect, yvect, 4 * pSprite->clipdist, 5120, -5120, CLIPMASK0, scratch);
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
    initsectp = pSprite->sector();
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


        uint32_t xDiff = abs((int32_t)((pPlayerActor->spr.x - x) >> 8));
        uint32_t yDiff = abs((int32_t)((pPlayerActor->spr.y - y) >> 8));

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

    int nAngle;

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

int GetWallNormal(walltype* pWall)
{
	auto delta = pWall->delta();

    int nAngle = GetMyAngle(delta.X, delta.y);
    return (nAngle + 512) & kAngleMask;
}

void WheresMyMouth(int nPlayer, vec3_t* pos, sectortype **sectnum)
{
    auto pActor = PlayerList[nPlayer].Actor();
	auto pSprite = &pActor->s();

    int height = GetActorHeight(pActor) >> 1;

    *sectnum = pSprite->sector();
    *pos = pSprite->pos;
    pos->z -= height;

    Collision scratch;
    clipmove(*pos, sectnum,
        bcos(pSprite->ang, 7),
        bsin(pSprite->ang, 7),
        5120, 1280, 1280, CLIPMASK1, scratch);
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
    DExhumedActor* pActor = nBodyGunSprite[nCurBodyGunNum];
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
            pSprite->cstat = CSTAT_SPRITE_INVISIBLE;
        }
		else
			pSprite = &pActor->s();


        nCurBodyNum++;
        if (nCurBodyNum >= 50) {
            nCurBodyNum = 0;
        }
    } while (pSprite->cstat & CSTAT_SPRITE_BLOCK_ALL);

    if (nBodyTotal < 50) {
        nBodyTotal++;
    }

    pSprite->cstat = 0;
    return pActor;
}

DExhumedActor* GrabChunkSprite()
{
    DExhumedActor* pActor = nChunkSprite[nCurChunkNum];

    if (pActor == nullptr)
    {
        pActor = insertActor(0, 899);
		nChunkSprite[nCurChunkNum] = pActor;
    }
    else if (pActor->spr.statnum)
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

    pActor->spr.cstat = CSTAT_SPRITE_YCENTER;

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

    ChangeActorSect(actor, pSrcSpr->sector());

    pSprite->cstat = CSTAT_SPRITE_YCENTER;
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

    auto pSector = pSprite->sector();
    pSprite->pal = pSector->ceilingpal;

    auto nVal = movesprite(pActor, pSprite->xvel << 10, pSprite->yvel << 10, pSprite->zvel, 2560, -2560, CLIPMASK1);

    if (pSprite->z >= pSector->floorz)
    {
        // re-grab this variable as it may have changed in movesprite(). Note the check above is against the value *before* movesprite so don't change it.
        pSector = pSprite->sector();

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->z = pSector->floorz;
    }
    else
    {
        if (!nVal.type && !nVal.exbits)
            return;

        int nAngle;

        if (nVal.exbits & kHitAux2)
        {
            pSprite->cstat = CSTAT_SPRITE_INVISIBLE;
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
                nAngle = nVal.actor()->spr.ang;
            }
            else if (nVal.type == kHitWall)
            {
                nAngle = GetWallNormal(nVal.hitWall);
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
        if (!((*ppEnemy)->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)) {
            *ppEnemy = nullptr;
        }
    }

    return *ppEnemy;
}
END_PS_NS
