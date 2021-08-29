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
#include "bloodactor.h"

BEGIN_BLD_NS


void fxFlameLick(DBloodActor* actor, int) // 0
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    XSPRITE *pXSprite = &actor->x();
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
        auto pFX = gFX.fxSpawnActor(FX_32, pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            pFX->xvel() = actor->xvel() + Random2(-dx);
            pFX->yvel() = actor->yvel() + Random2(-dy);
            pFX->zvel() = actor->zvel() - Random(0x1aaaa);
        }
    }
    if (pXSprite->burnTime > 0)
        evPostActor(actor, 5, kCallbackFXFlameLick);
}

void Remove(DBloodActor* actor, int) // 1
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    evKillActor(actor, kCallbackFXFlareSpark);
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    sfxKill3DSound(pSprite, 0, -1);
    DeleteSprite(actor);
}

void FlareBurst(DBloodActor* actor, int) // 2
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    int nAngle = getangle(actor->xvel(), actor->yvel());
    int nRadius = 0x55555;
    for (int i = 0; i < 8; i++)
    {
        auto spawnedactor = actSpawnSprite(actor, 5);
        spritetype *pSpawn = &spawnedactor->s();
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
        spawnedactor->xvel() += dx;
        spawnedactor->yvel() += dy;
        spawnedactor->zvel() += dz;
        evPostActor(spawnedactor, 960, kCallbackRemove);
    }
    evPostActor(actor, 0, kCallbackRemove);
}

void fxFlareSpark(DBloodActor* actor, int) // 3
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    auto pFX = gFX.fxSpawnActor(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->xvel() = actor->xvel() + Random2(0x1aaaa);
        pFX->yvel() = actor->yvel() + Random2(0x1aaaa);
        pFX->zvel() = actor->zvel() - Random(0x1aaaa);
    }
    evPostActor(actor, 4, kCallbackFXFlareSpark);
}

void fxFlareSparkLite(DBloodActor* actor, int) // 4
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    auto pFX = gFX.fxSpawnActor(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->xvel() = actor->xvel() + Random2(0x1aaaa);
        pFX->yvel() = actor->yvel() + Random2(0x1aaaa);
        pFX->zvel() = actor->zvel() - Random(0x1aaaa);
    }
    evPostActor(actor, 12, kCallbackFXFlareSparkLite);
}

void fxZombieBloodSpurt(DBloodActor* actor, int) // 5
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    int nXSprite = pSprite->extra;
    assert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE *pXSprite = &actor->x();
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    auto pFX = gFX.fxSpawnActor(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
    if (pFX)
    {
        pFX->xvel() = actor->xvel() + Random2(0x11111);
        pFX->yvel() = actor->yvel() + Random2(0x11111);
        pFX->zvel() = actor->zvel() - 0x6aaaa;
    }
    if (pXSprite->data1 > 0)
    {
        evPostActor(actor, 4, kCallbackFXZombieSpurt);
        pXSprite->data1 -= 4;
    }
    else if (pXSprite->data2 > 0)
    {
        evPostActor(actor, 60, kCallbackFXZombieSpurt);
        pXSprite->data1 = 40;
        pXSprite->data2--;
    }
}

void fxBloodSpurt(DBloodActor* actor, int) // 6
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    auto pFX = gFX.fxSpawnActor(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->s().ang = 0;
        pFX->xvel() = actor->xvel()>>8;
        pFX->yvel() = actor->yvel()>>8;
        pFX->zvel() = actor->zvel()>>8;
    }
    evPostActor(actor, 6, kCallbackFXBloodSpurt);
}


void fxArcSpark(DBloodActor* actor, int) // 7
{
    if (!actor) return;
    spritetype* pSprite = &actor->s();
    auto pFX = gFX.fxSpawnActor(FX_15, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->xvel() = actor->xvel() + Random2(0x10000);
        pFX->yvel() = actor->yvel() + Random2(0x10000);
        pFX->zvel() = actor->zvel() - Random(0x1aaaa);
    }
    evPostActor(actor, 3, kCallbackFXArcSpark);
}


void fxDynPuff(DBloodActor* actor, int) // 8
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    if (actor->zvel())
    {
        int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>2;
        int x = pSprite->x + MulScale(nDist, Cos(pSprite->ang-512), 30);
        int y = pSprite->y + MulScale(nDist, Sin(pSprite->ang-512), 30);
        int z = pSprite->z;
        auto pFX = gFX.fxSpawnActor(FX_7, pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            pFX->xvel() = actor->xvel();
            pFX->yvel() = actor->yvel();
            pFX->zvel() = actor->zvel();
        }
    }
    evPostActor(actor, 12, kCallbackFXDynPuff);
}

void Respawn(DBloodActor* actor, int) // 9
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    assert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    
    if (pSprite->statnum != kStatRespawn && pSprite->statnum != kStatThing) {
        viewSetSystemMessage("Sprite #%d is not on Respawn or Thing list\n", actor->GetIndex());
        return;
    } else if (!(pSprite->flags & kHitagRespawn)) {
        viewSetSystemMessage("Sprite #%d does not have the respawn attribute\n", actor->GetIndex());
        return;
    }

    switch (pXSprite->respawnPending) {
        case 1: {
            int nTime = MulScale(actGetRespawnTime(actor), 0x4000, 16);
            pXSprite->respawnPending = 2;
            evPostActor(actor, nTime, kCallbackRespawn);
            break;
        }
        case 2: {
            int nTime = MulScale(actGetRespawnTime(actor), 0x2000, 16);
            pXSprite->respawnPending = 3;
            evPostActor(actor, nTime, kCallbackRespawn);
            break;
        }
        case 3: {
            assert(pSprite->owner != kStatRespawn);
            assert(pSprite->owner >= 0 && pSprite->owner < kMaxStatus);
            ChangeSpriteStat(actor->s().index, pSprite->owner);
            pSprite->type = pSprite->inittype;
            pSprite->owner = -1;
            pSprite->flags &= ~kHitagRespawn;
            actor->xvel() = actor->yvel() = actor->zvel() = 0;
            pXSprite->respawnPending = 0;
            pXSprite->burnTime = 0;
            pXSprite->isTriggered = 0;
            if (actor->IsDudeActor()) 
            {
                int nType = pSprite->type-kDudeBase;
                pSprite->x = actor->basePoint().x;
                pSprite->y = actor->basePoint().y;
                pSprite->z = actor->basePoint().z;
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
                        seqSpawn(genDudeSeqStartId(actor), 3, pSprite->extra, -1);
                        break;
                }
                
                // return dude to the patrol state
                if (gModernMap && pXSprite->dudeFlag4) {
                    pXSprite->data3 = 0;
                    actor->SetTarget(nullptr);
                }
                #else
                pSprite->clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
                pXSprite->health = getDudeInfo(nType + kDudeBase)->startHealth << 4;
                if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
                    seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, 3, pSprite->extra, -1);
                #endif
                aiInitSprite(actor);
                pXSprite->key = 0;
            } else if (pSprite->type == kThingTNTBarrel) {
                pSprite->cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
            }

            gFX.fxSpawnActor(FX_29, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
            sfxPlay3DSound(pSprite, 350, -1, 0);
            break;
        }
    }
}

void PlayerBubble(DBloodActor* actor, int) // 10
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
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
            auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
            if (pFX)
            {
                pFX->xvel() = actor->xvel() + Random2(0x1aaaa);
                pFX->yvel() = actor->yvel() + Random2(0x1aaaa);
                pFX->zvel() = actor->zvel() + Random2(0x1aaaa);
            }
        }
        evPostActor(actor, 4, kCallbackPlayerBubble);
    }
}

void EnemyBubble(DBloodActor* actor, int) // 11
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < (abs(actor->zvel())>>18); i++)
    {
        int nDist = (pSprite->xrepeat*(tileWidth(pSprite->picnum)/2))>>2;
        int nAngle = Random(2048);
        int x = pSprite->x + MulScale(nDist, Cos(nAngle), 30);
        int y = pSprite->y + MulScale(nDist, Sin(nAngle), 30);
        int z = bottom-Random(bottom-top);
        auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            pFX->xvel() = actor->xvel() + Random2(0x1aaaa);
            pFX->yvel() = actor->yvel() + Random2(0x1aaaa);
            pFX->zvel() = actor->zvel() + Random2(0x1aaaa);
        }
    }
    evPostActor(actor, 4, kCallbackEnemeyBubble);
}

void CounterCheck(DBloodActor*, int nSector) // 12
{
    if (nSector < 0 || nSector >= kMaxSectors) return;
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
        evPostSector(nSector, 5, kCallbackCounterCheck);
        return;
    } else {
        //pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
        trTriggerSector(nSector, pXSector, kCmdOn);
        pXSector->locked = 1; //lock sector, so it can be opened again later
    }
}


void FinishHim(DBloodActor* actor, int) // 13
{
    if (!actor) return;
    spritetype* pSprite = &actor->s();
    if (actor->IsPlayerActor() && playerSeqPlaying(&gPlayer[pSprite->type - kDudePlayer1], 16) && actor == gMe->actor())
        sndStartSample(3313, -1, 1, 0);
}

void fxBloodBits(DBloodActor* actor, int) // 14
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;
    GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = pSprite->x+MulScale(nDist, Cos(nAngle), 28);
    int y = pSprite->y+MulScale(nDist, Sin(nAngle), 28);
    gFX.fxSpawnActor(FX_48, pSprite->sectnum, x, y, pSprite->z, 0);
    if (pSprite->ang == 1024)
    {
        int nChannel = 28+(pSprite->index&2);
        assert(nChannel < 32);
        sfxPlay3DSound(pSprite, 385, nChannel, 1);
    }
    if (Chance(0x5000))
    {
        auto pFX = gFX.fxSpawnActor(FX_36, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->s().ang = nAngle;
    }
    gFX.remove(actor);
}


void fxTeslaAlt(DBloodActor* actor, int) // 15
{
    if (!actor) return;
    spritetype* pSprite = &actor->s();
    auto pFX = gFX.fxSpawnActor(FX_49, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->xvel() = actor->xvel() + Random2(0x1aaaa);
        pFX->yvel() = actor->yvel() + Random2(0x1aaaa);
        pFX->zvel() = actor->zvel() - Random(0x1aaaa);
    }
    evPostActor(actor, 3, kCallbackFXTeslaAlt);
}


int tommySleeveSnd[] = { 608, 609, 611 }; // unused?
int sawedOffSleeveSnd[] = { 610, 612 };

void fxBouncingSleeve(DBloodActor* actor, int) // 16
{
    if (!actor) return;
    spritetype* pSprite = &actor->s(); 
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;

    GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist, CLIPMASK0);
    int top, bottom; GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ - bottom;
    
    int zv = actor->zvel() - velFloor[pSprite->sectnum];
    
    if (actor->zvel() == 0) sleeveStopBouncing(actor);
    else if (zv > 0) {
        actFloorBounceVector((int*)& actor->xvel(), (int*)& actor->yvel(), &zv, pSprite->sectnum, 0x9000);
        actor->zvel() = zv;
        if (velFloor[pSprite->sectnum] == 0 && abs(actor->zvel()) < 0x20000)  {
            sleeveStopBouncing(actor);
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


void sleeveStopBouncing(DBloodActor* actor) 
{
    auto pSprite = &actor->s();
    actor->xvel() = actor->yvel() = actor->zvel() = 0;
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


void returnFlagToBase(DBloodActor* actor, int) // 17
{
    if (!actor) return;
    spritetype* pSprite = &actor->s();
    auto owner = actor->GetOwner();
    if (owner)
    {
        spritetype* pOwner = &owner->s();
        XSPRITE* pXOwner = &owner->x();
        switch (pSprite->type) 
        {
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
    evPostActor(actor, 0, kCallbackRemove);
}

void fxPodBloodSpray(DBloodActor* actor, int) // 18
{
    if (!actor) return;
    spritetype* pSprite = &actor->s();
    DBloodActor* pFX;
    if (pSprite->type == 53)
        pFX = gFX.fxSpawnActor(FX_53, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    else
        pFX = gFX.fxSpawnActor(FX_54, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->s().ang = 0;
        pFX->xvel() = actor->xvel() >> 8;
        pFX->yvel() = actor->yvel() >> 8;
        pFX->zvel() = actor->zvel() >> 8;
    }
    evPostActor(actor, 6, kCallbackFXPodBloodSpray);
}

void fxPodBloodSplat(DBloodActor* actor, int) // 19
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;

    GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist, CLIPMASK0);
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
    DBloodActor *pFX = NULL;
    if (pSprite->type == 53 || pSprite->type == kThingPodGreenBall)
    {
        if (Chance(0x500) || pSprite->type == kThingPodGreenBall)
            pFX = gFX.fxSpawnActor(FX_55, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->s().ang = nAngle;
    }
    else
    {
        pFX = gFX.fxSpawnActor(FX_32, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->s().ang = nAngle;
    }
    gFX.remove(actor);
}



void LeechStateTimer(DBloodActor* actor, int) // 20
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
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

void sub_76A08(DBloodActor *actor, spritetype *pSprite2, PLAYER *pPlayer) // ???
{
    int top, bottom;
    auto pSprite = &actor->s();
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->x = pSprite2->x;
    pSprite->y = pSprite2->y;
    pSprite->z = sector[pSprite2->sectnum].floorz-(bottom-pSprite->z);
    pSprite->ang = pSprite2->ang;
    ChangeActorSect(actor, pSprite2->sectnum);
    sfxPlay3DSound(pSprite2, 201, -1, 0);
    actor->xvel() = actor->yvel() = actor->zvel() = 0;
    viewBackupSpriteLoc(pSprite->index, pSprite);
    if (pPlayer)
    {
        playerResetInertia(pPlayer);
        pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
    }
}

void DropVoodooCb(DBloodActor* actor, int) // unused
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    auto Owner = actor->GetOwner();
    if (Owner == nullptr)
    {
        evPostActor(actor, 0, kCallbackRemove);
        return;
    }
    spritetype *pOwner = &Owner->s();
    PLAYER *pPlayer;
    if (IsPlayerSprite(pOwner))
        pPlayer = &gPlayer[pOwner->type-kDudePlayer1];
    else
        pPlayer = NULL;
    if (!pPlayer)
    {
        evPostActor(actor, 0, kCallbackRemove);
        return;
    }
    pSprite->ang = getangle(pOwner->x-pSprite->x, pOwner->y-pSprite->y);
    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE *pXSprite = &actor->x();
        if (pXSprite->data1 == 0)
        {
            evPostActor(actor, 0, kCallbackRemove);
            return;
        }

        BloodStatIterator it(kStatDude);
        while (auto actor2 = it.Next())
        {
            auto nextactor = it.Peek();
            if (Owner == actor2)
                continue;
            spritetype *pSprite2 = &actor2->s();
            if (actor2->hasX())
            {
                XSPRITE *pXSprite2 = &actor2->x();
                PLAYER *pPlayer2;
                if (actor2->IsPlayerActor())
                    pPlayer2 = &gPlayer[pSprite2->type-kDudePlayer1];
                else
                    pPlayer2 = nullptr;

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
                        if (Chance(t) || nextactor == nullptr)
                        {
                            int nDmg = actDamageSprite(actor, actor2, kDamageSpirit, pXSprite->data1<<4);
                            pXSprite->data1 = ClipLow(pXSprite->data1-nDmg, 0);
                            sub_76A08(actor2, pSprite, pPlayer2);
                            evPostActor(actor, 0, kCallbackRemove);
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
                        if (vd && (Chance(vd) || nextactor == nullptr))
                        {
                            sub_76A08(actor2, pSprite, NULL);
                            evPostActor(actor, 0, kCallbackRemove);
                            return;
                        }
                    }
                }
            }
        }
        pXSprite->data1 = ClipLow(pXSprite->data1-1, 0);
        evPostActor(actor, 0, kCallbackRemove);
    }
}

void callbackCondition(DBloodActor* actor, int)
{
    XSPRITE* pXSprite = &actor->x();
    if (pXSprite->isTriggered) return;

    TRCONDITION* pCond = &gCondition[pXSprite->sysData1];
    for (unsigned i = 0; i < pCond->length; i++) {
        EVENT evn;  
        evn.type = pCond->obj[i].type;
        evn.actor = pCond->obj[i].actor;
        evn.index_ = pCond->obj[i].index_;
        evn.cmd = pCond->obj[i].cmd; 
        evn.funcID = kCallbackCondition;
        useCondition(actor, evn);
    }

    evPostActor(actor, pXSprite->busyTime, kCallbackCondition);
    return;
}

void(*gCallback[kCallbackMax])(DBloodActor*, int) =
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
