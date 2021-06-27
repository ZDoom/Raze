//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "blood.h"

BEGIN_BLD_NS


void fxFlameLick(int nSprite) // 0
{
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < 3; i++)
    {
        int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>3;
        int nAngle = Random(2048);
        int dx = MulScale(nDist, Cos(nAngle), 30);
        int dy = MulScale(nDist, Sin(nAngle), 30);
        int x = pSprite->x + dx;
        int y = pSprite->y + dy;
        int z = bottom-Random(bottom-top);
        spritetype *pFX = gFX.fxSpawn(FX_32, pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            xvel[pFX->index] = xvel[nSprite] + Random2(-dx);
            yvel[pFX->index] = yvel[nSprite] + Random2(-dy);
            zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
        }
    }
    if (pXSprite->burnTime > 0)
        evPost(nSprite, 3, 5, kCallbackFXFlameLick);
}

void Remove(int nSprite) // 1
{
    spritetype *pSprite = &sprite[nSprite];
    evKill(nSprite, 3);
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    sfxKill3DSound(pSprite, 0, -1);
    DeleteSprite(nSprite);
}

void FlareBurst(int nSprite) // 2
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;
    for (int i = 0; i < 8; i++)
    {
        spritetype *pSpawn = actSpawnSprite(pSprite, 5);
        pSpawn->picnum = 2424;
        pSpawn->shade = -128;
        pSpawn->xrepeat = pSpawn->yrepeat = 32;
        pSpawn->type = kMissileFlareAlt;
        pSpawn->clipdist = 2;
        pSpawn->owner = pSprite->owner;
        int nAngle2 = (i<<11)/8;
        int dx = 0;
        int dy = mulscale30r(nRadius, Sin(nAngle2));
        int dz = mulscale30r(nRadius, -Cos(nAngle2));
        if (i&1)
        {
            dy >>= 1;
            dz >>= 1;
        }
        RotateVector(&dx, &dy, nAngle);
        xvel[pSpawn->index] += dx;
        yvel[pSpawn->index] += dy;
        zvel[pSpawn->index] += dz;
        evPost(pSpawn->index, 3, 960, kCallbackRemove);
    }
    evPost(nSprite, 3, 0, kCallbackRemove);
}

void fxFlareSpark(int nSprite) // 3
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 4, kCallbackFXFlareSpark);
}

void fxFlareSparkLite(int nSprite) // 4
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 12, kCallbackFXFlareSparkLite);
}

void fxZombieBloodSpurt(int nSprite) // 5
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    assert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    spritetype *pFX = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x11111);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x11111);
        zvel[pFX->index] = zvel[nSprite] - 0x6aaaa;
    }
    if (pXSprite->data1 > 0)
    {
        evPost(nSprite, 3, 4, kCallbackFXZombieSpurt);
        pXSprite->data1 -= 4;
    }
    else if (pXSprite->data2 > 0)
    {
        evPost(nSprite, 3, 60, kCallbackFXZombieSpurt);
        pXSprite->data1 = 40;
        pXSprite->data2--;
    }
}

void fxBloodSpurt(int nSprite) // 6
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->ang = 0;
        xvel[pFX->index] = xvel[nSprite]>>8;
        yvel[pFX->index] = yvel[nSprite]>>8;
        zvel[pFX->index] = zvel[nSprite]>>8;
    }
    evPost(nSprite, 3, 6, kCallbackFXBloodSpurt);
}


void fxArcSpark(int nSprite) // 7
{
    spritetype* pSprite = &sprite[nSprite];
    spritetype* pFX = gFX.fxSpawn(FX_15, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x10000);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x10000);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 3, kCallbackFXArcSpark);
}


void fxDynPuff(int nSprite) // 8
{
    spritetype *pSprite = &sprite[nSprite];
    if (zvel[nSprite])
    {
        int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>2;
        int x = pSprite->x + MulScale(nDist, Cos(pSprite->ang-512), 30);
        int y = pSprite->y + MulScale(nDist, Sin(pSprite->ang-512), 30);
        int z = pSprite->z;
        spritetype *pFX = gFX.fxSpawn(FX_7, pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            xvel[pFX->index] = xvel[nSprite];
            yvel[pFX->index] = yvel[nSprite];
            zvel[pFX->index] = zvel[nSprite];
        }
    }
    evPost(nSprite, 3, 12, kCallbackFXDynPuff);
}

void Respawn(int nSprite) // 9
{
    spritetype *pSprite = &sprite[nSprite];
    assert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    
    if (pSprite->statnum != kStatRespawn && pSprite->statnum != kStatThing) {
        viewSetSystemMessage("Sprite #%d is not on Respawn or Thing list\n", nSprite);
        return;
    } else if (!(pSprite->flags & kHitagRespawn)) {
        viewSetSystemMessage("Sprite #%d does not have the respawn attribute\n", nSprite);
        return;
    }

    switch (pXSprite->respawnPending) {
        case 1: {
            int nTime = MulScale(actGetRespawnTime(pSprite), 0x4000, 16);
            pXSprite->respawnPending = 2;
            evPost(nSprite, 3, nTime, kCallbackRespawn);
            break;
        }
        case 2: {
            int nTime = MulScale(actGetRespawnTime(pSprite), 0x2000, 16);
            pXSprite->respawnPending = 3;
            evPost(nSprite, 3, nTime, kCallbackRespawn);
            break;
        }
        case 3: {
            assert(pSprite->owner != kStatRespawn);
            assert(pSprite->owner >= 0 && pSprite->owner < kMaxStatus);
            ChangeSpriteStat(nSprite, pSprite->owner);
            pSprite->type = pSprite->inittype;
            pSprite->owner = -1;
            pSprite->flags &= ~kHitagRespawn;
            xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
            pXSprite->respawnPending = 0;
            pXSprite->burnTime = 0;
            pXSprite->isTriggered = 0;
            if (IsDudeSprite(pSprite)) {
                int nType = pSprite->type-kDudeBase;
                pSprite->x = baseSprite[nSprite].x;
                pSprite->y = baseSprite[nSprite].y;
                pSprite->z = baseSprite[nSprite].z;
                pSprite->cstat |= 0x1101;
                #ifdef NOONE_EXTENSIONS
                if (!gModernMap || pXSprite->sysData2 <= 0) pXSprite->health = dudeInfo[pSprite->type - kDudeBase].startHealth << 4;
                else pXSprite->health = ClipRange(pXSprite->sysData2 << 4, 1, 65535);

                switch (pSprite->type) {
                    default:
                        pSprite->clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
                        if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
                            seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, 3, pSprite->extra, -1);
                        break;
                    case kDudeModernCustom:
                        seqSpawn(genDudeSeqStartId(pXSprite), 3, pSprite->extra, -1);
                        break;
                }
                
                // return dude to the patrol state
                if (gModernMap && pXSprite->dudeFlag4) {
                    pXSprite->data3 = 0;
                    pXSprite->target = -1;
                }
                #else
                pSprite->clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
                pXSprite->health = getDudeInfo(nType + kDudeBase)->startHealth << 4;
                if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
                    seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, 3, pSprite->extra, -1);
                #endif
                aiInitSprite(pSprite);
                pXSprite->key = 0;
            } else if (pSprite->type == kThingTNTBarrel) {
                pSprite->cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
            }

            gFX.fxSpawn(FX_29, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
            sfxPlay3DSound(pSprite, 350, -1, 0);
            break;
        }
    }
}

void PlayerBubble(int nSprite) // 10
{
    spritetype *pSprite = &sprite[nSprite];
    if (IsPlayerSprite(pSprite))
    {
        PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
        assert(pPlayer != NULL);
        if (!pPlayer->bubbleTime)
            return;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        for (int i = 0; i < (pPlayer->bubbleTime>>6); i++)
        {
            int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>2;
            int nAngle = Random(2048);
            int x = pSprite->x + MulScale(nDist, Cos(nAngle), 30);
            int y = pSprite->y + MulScale(nDist, Sin(nAngle), 30);
            int z = bottom-Random(bottom-top);
            spritetype *pFX = gFX.fxSpawn((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
            if (pFX)
            {
                xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
                yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
                zvel[pFX->index] = zvel[nSprite] + Random2(0x1aaaa);
            }
        }
        evPost(nSprite, 3, 4, kCallbackPlayerBubble);
    }
}

void EnemyBubble(int nSprite) // 11
{
    spritetype *pSprite = &sprite[nSprite];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < (abs(zvel[nSprite])>>18); i++)
    {
        int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>2;
        int nAngle = Random(2048);
        int x = pSprite->x + MulScale(nDist, Cos(nAngle), 30);
        int y = pSprite->y + MulScale(nDist, Sin(nAngle), 30);
        int z = bottom-Random(bottom-top);
        spritetype *pFX = gFX.fxSpawn((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
            yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
            zvel[pFX->index] = zvel[nSprite] + Random2(0x1aaaa);
        }
    }
    evPost(nSprite, 3, 4, kCallbackEnemeyBubble);
}

void CounterCheck(int nSector) // 12
{
    assert(nSector >= 0 && nSector < kMaxSectors);
    if (sector[nSector].type != kSectorCounter) return;
    if (sector[nSector].extra <= 0) return;
    
    XSECTOR *pXSector = &xsector[sector[nSector].extra];
    int nReq = pXSector->waitTimeA; int nType = pXSector->data; int nCount = 0;
    if (!nType || !nReq) return;
    
    int nSprite;
    SectIterator it(nSector);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        if (sprite[nSprite].type == nType) nCount++;
    }
        
    if (nCount < nReq) {
        evPost(nSector, 6, 5, kCallbackCounterCheck);
        return;
    } else {
        //pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
        trTriggerSector(nSector, pXSector, kCmdOn);
        pXSector->locked = 1; //lock sector, so it can be opened again later
    }
}


void FinishHim(int nSprite) // 13
{
    spritetype* pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    XSPRITE* pXSprite = &xsprite[nXSprite];
    if (IsPlayerSprite(pSprite) && playerSeqPlaying(&gPlayer[pSprite->type - kDudePlayer1], 16) && pXSprite->target == gMe->nSprite)
        sndStartSample(3313, -1, 1, 0);
}

void fxBloodBits(int nSprite) // 14
{
    spritetype *pSprite = &sprite[nSprite];
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = pSprite->x+MulScale(nDist, Cos(nAngle), 28);
    int y = pSprite->y+MulScale(nDist, Sin(nAngle), 28);
    gFX.fxSpawn(FX_48, pSprite->sectnum, x, y, pSprite->z, 0);
    if (pSprite->ang == 1024)
    {
        int nChannel = 28+(pSprite->index&2);
        assert(nChannel < 32);
        sfxPlay3DSound(pSprite, 385, nChannel, 1);
    }
    if (Chance(0x5000))
    {
        spritetype *pFX = gFX.fxSpawn(FX_36, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->ang = nAngle;
    }
    gFX.remove(nSprite);
}


void fxTeslaAlt(int nSprite) // 15
{
    spritetype* pSprite = &sprite[nSprite];
    spritetype* pFX = gFX.fxSpawn(FX_49, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 3, kCallbackFXTeslaAlt);
}


int tommySleeveSnd[] = { 608, 609, 611 }; // unused?
int sawedOffSleeveSnd[] = { 610, 612 };

void fxBouncingSleeve(int nSprite) // 16
{
    spritetype* pSprite = &sprite[nSprite]; int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom; GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ - bottom;
    
    int zv = zvel[nSprite] - velFloor[pSprite->sectnum];
    
    if (zvel[nSprite] == 0) sleeveStopBouncing(pSprite);
    else if (zv > 0) {
        actFloorBounceVector((int*)& xvel[nSprite], (int*)& yvel[nSprite], &zv, pSprite->sectnum, 0x9000);
        zvel[nSprite] = zv;
        if (velFloor[pSprite->sectnum] == 0 && abs(zvel[nSprite]) < 0x20000)  {
            sleeveStopBouncing(pSprite);
            return;
        }

        int nChannel = 28 + (pSprite->index & 2);
        assert(nChannel < 32);
        
        // tommy sleeve
        if (pSprite->type >= 37 && pSprite->type <= 39) {
            Random(3); 
            sfxPlay3DSound(pSprite, 608 + Random(2), nChannel, 1);
        
        // sawed-off sleeve
        } else {
            sfxPlay3DSound(pSprite, sawedOffSleeveSnd[Random(2)], nChannel, 1);
        }
    }   

}


void sleeveStopBouncing(spritetype* pSprite) {
    xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
    if (pSprite->extra > 0) seqKill(3, pSprite->extra);
    sfxKill3DSound(pSprite, -1, -1);

    switch (pSprite->type) {
    case 37:
    case 38:
    case 39:
        pSprite->picnum = 2465;
        break;
    case 40:
    case 41:
    case 42:
        pSprite->picnum = 2464;
        break;
    }

    pSprite->type = 51;
    pSprite->xrepeat = pSprite->yrepeat = 10;
}


void returnFlagToBase(int nSprite) // 17
{
    spritetype* pSprite = &sprite[nSprite];
    if (pSprite->owner >= 0 && pSprite->owner < kMaxSprites)
    {
        spritetype* pOwner = &sprite[pSprite->owner];
        XSPRITE* pXOwner = &xsprite[pOwner->extra];
        switch (pSprite->type) {
            case kItemFlagA:
                trTriggerSprite(pOwner->index, pXOwner, kCmdOn);
                sndStartSample(8003, 255, 2, 0);
                gBlueFlagDropped = false;
                viewSetMessage("Blue Flag returned to base.");
                break;
            case kItemFlagB:
                trTriggerSprite(pOwner->index, pXOwner, kCmdOn);
                sndStartSample(8002, 255, 2, 0);
                gRedFlagDropped = false;
                viewSetMessage("Red Flag returned to base.");
                break;
        }
    }
    evPost(pSprite->index, 3, 0, kCallbackRemove);
}

void fxPodBloodSpray(int nSprite) // 18
{
    spritetype* pSprite = &sprite[nSprite];
    spritetype* pFX;
    if (pSprite->type == 53)
        pFX = gFX.fxSpawn(FX_53, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    else
        pFX = gFX.fxSpawn(FX_54, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->ang = 0;
        xvel[pFX->index] = xvel[nSprite] >> 8;
        yvel[pFX->index] = yvel[nSprite] >> 8;
        zvel[pFX->index] = zvel[nSprite] >> 8;
    }
    evPost(nSprite, 3, 6, kCallbackFXPodBloodSpray);
}

void fxPodBloodSplat(int nSprite) // 19
{
    spritetype *pSprite = &sprite[nSprite];
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = pSprite->x+MulScale(nDist, Cos(nAngle), 28);
    int y = pSprite->y+MulScale(nDist, Sin(nAngle), 28);
    if (pSprite->ang == 1024)
    {
        int nChannel = 28+(pSprite->index&2);
        assert(nChannel < 32);
        sfxPlay3DSound(pSprite, 385, nChannel, 1);
    }
    spritetype *pFX = NULL;
    if (pSprite->type == 53 || pSprite->type == kThingPodGreenBall)
    {
        if (Chance(0x500) || pSprite->type == kThingPodGreenBall)
            pFX = gFX.fxSpawn(FX_55, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->ang = nAngle;
    }
    else
    {
        pFX = gFX.fxSpawn(FX_32, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->ang = nAngle;
    }
    gFX.remove(nSprite);
}



void LeechStateTimer(int nSprite) // 20
{
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->statnum == kStatThing && !(pSprite->flags & 32)) {
        switch (pSprite->type) {
            case kThingDroppedLifeLeech:
            #ifdef NOONE_EXTENSIONS
            case kModernThingEnemyLifeLeech:
            #endif
                xsprite[pSprite->extra].stateTimer = 0;
                break;
        }
    }
}

void sub_76A08(spritetype *pSprite, spritetype *pSprite2, PLAYER *pPlayer) // ???
{
    int top, bottom;
    int nSprite = pSprite->index;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->x = pSprite2->x;
    pSprite->y = pSprite2->y;
    pSprite->z = sector[pSprite2->sectnum].floorz-(bottom-pSprite->z);
    pSprite->ang = pSprite2->ang;
    ChangeSpriteSect(nSprite, pSprite2->sectnum);
    sfxPlay3DSound(pSprite2, 201, -1, 0);
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
    viewBackupSpriteLoc(nSprite, pSprite);
    if (pPlayer)
    {
        playerResetInertia(pPlayer);
        pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
    }
}

void DropVoodooCb(int nSprite) // unused
{
    spritetype *pSprite = &sprite[nSprite];
    int nOwner = pSprite->owner;
    if (nOwner < 0 || nOwner >= kMaxSprites)
    {
        evPost(nSprite, 3, 0, kCallbackRemove);
        return;
    }
    spritetype *pOwner = &sprite[nOwner];
    PLAYER *pPlayer;
    if (IsPlayerSprite(pOwner))
        pPlayer = &gPlayer[pOwner->type-kDudePlayer1];
    else
        pPlayer = NULL;
    if (!pPlayer)
    {
        evPost(nSprite, 3, 0, kCallbackRemove);
        return;
    }
    pSprite->ang = getangle(pOwner->x-pSprite->x, pOwner->y-pSprite->y);
    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE *pXSprite = &xsprite[nXSprite];
        if (pXSprite->data1 == 0)
        {
            evPost(nSprite, 3, 0, kCallbackRemove);
            return;
        }
        int nSprite2;
        StatIterator it(kStatDude);
        while ((nSprite2 = it.NextIndex()) >= 0)
        {
            int nNextSprite = it.PeekIndex();
            if (nOwner == nSprite2)
                continue;
            spritetype *pSprite2 = &sprite[nSprite2];
            int nXSprite2 = pSprite2->extra;
            if (nXSprite2 > 0 && nXSprite2 < kMaxXSprites)
            {
                XSPRITE *pXSprite2 = &xsprite[nXSprite2];
                PLAYER *pPlayer2;
                if (IsPlayerSprite(pSprite2))
                    pPlayer2 = &gPlayer[pSprite2->type-kDudePlayer1];
                else
                    pPlayer2 = NULL;
                if (pXSprite2->health > 0 && (pPlayer2 || pXSprite2->key == 0))
                {
                    if (pPlayer2)
                    {
                        if (gGameOptions.nGameType == 1)
                            continue;
                        if (gGameOptions.nGameType == 3 && pPlayer->teamId == pPlayer2->teamId)
                            continue;
                        int t = 0x8000/ClipLow(gNetPlayers-1, 1);
                        if (!powerupCheck(pPlayer2, kPwUpDeathMask))
                            t += ((3200-pPlayer2->armor[2])<<15)/3200;
                        if (Chance(t) || nNextSprite < 0)
                        {
                            int nDmg = actDamageSprite(nOwner, pSprite2, kDamageSpirit, pXSprite->data1<<4);
                            pXSprite->data1 = ClipLow(pXSprite->data1-nDmg, 0);
                            sub_76A08(pSprite2, pSprite, pPlayer2);
                            evPost(nSprite, 3, 0, kCallbackRemove);
                            return;
                        }
                    }
                    else
                    {
                        int vd = 0x2666;
                        switch (pSprite2->type)
                        {
                        case kDudeBoneEel:
                        case kDudeBat:
                        case kDudeRat:
                        case kDudeTinyCaleb:
                        case kDudeBeast:
                            vd = 0x147;
                            break;
                        case kDudeZombieAxeBuried:
                        case kDudePodGreen:
                        case kDudeTentacleGreen:
                        case kDudePodFire:
                        case kDudeTentacleFire:
                        case kDudePodMother:
                        case kDudeTentacleMother:
                        case kDudeCerberusTwoHead:
                        case kDudeCerberusOneHead:
                        case kDudeTchernobog:
                        case kDudeBurningInnocent:
                        case kDudeBurningCultist:
                        case kDudeBurningZombieAxe:
                        case kDudeBurningZombieButcher:
                        case kDudeCultistReserved:
                        case kDudeZombieAxeLaying:
                        case kDudeInnocent:
                        case kDudeBurningTinyCaleb:
                        case kDudeBurningBeast:
                            vd = 0;
                            break;
                        }
                        if (vd && (Chance(vd) || nNextSprite < 0))
                        {
                            sub_76A08(pSprite2, pSprite, NULL);
                            evPost(nSprite, 3, 0, kCallbackRemove);
                            return;
                        }
                    }
                }
            }
        }
        pXSprite->data1 = ClipLow(pXSprite->data1-1, 0);
        evPost(nSprite, 3, 0, kCallbackRemove);
    }
}

void callbackCondition(int nSprite) {
    
    XSPRITE* pXSprite = &xsprite[sprite[nSprite].extra];
    if (pXSprite->isTriggered) return;

    TRCONDITION* pCond = &gCondition[pXSprite->sysData1];
    for (unsigned i = 0; i < pCond->length; i++) {
        EVENT evn;  evn.index = pCond->obj[i].index;   evn.type = pCond->obj[i].type;
        evn.cmd = pCond->obj[i].cmd; evn.funcID = kCallbackCondition;
        useCondition(&sprite[pXSprite->reference], pXSprite, evn);
    }

    evPost(nSprite, OBJ_SPRITE, pXSprite->busyTime, kCallbackCondition);
    return;
}

void(*gCallback[kCallbackMax])(int) =
{
    fxFlameLick,
    Remove,
    FlareBurst,
    fxFlareSpark,
    fxFlareSparkLite,
    fxZombieBloodSpurt,
    fxBloodSpurt,
    fxArcSpark,
    fxDynPuff,
    Respawn,
    PlayerBubble,
    EnemyBubble,
    CounterCheck,
    FinishHim,
    fxBloodBits,
    fxTeslaAlt,
    fxBouncingSleeve,
    returnFlagToBase,
    fxPodBloodSpray,
    fxPodBloodSplat,
    LeechStateTimer,
    DropVoodooCb, // unused
    #ifdef NOONE_EXTENSIONS
    callbackUniMissileBurst, // the code is in nnexts.cpp
    callbackMakeMissileBlocking, // the code is in nnexts.cpp
    callbackGenDudeUpdate, // the code is in nnexts.cpp
    callbackCondition,
    #endif
};

END_BLD_NS
