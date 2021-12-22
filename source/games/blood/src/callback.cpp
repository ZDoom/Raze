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


void fxFlameLick(DBloodActor* actor, sectortype*) // 0
{
    if (!actor) return;
    XSPRITE *pXSprite = &actor->x();
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    for (int i = 0; i < 3; i++)
    {
        int nDist = (actor->spr.xrepeat*(tileWidth(actor->spr.picnum)/2))>>3;
        int nAngle = Random(2048);
        int dx = MulScale(nDist, Cos(nAngle), 30);
        int dy = MulScale(nDist, Sin(nAngle), 30);
        int x = actor->spr.pos.X + dx;
        int y = actor->spr.pos.Y + dy;
        int z = bottom-Random(bottom-top);
        auto pFX = gFX.fxSpawnActor(FX_32, actor->spr.sector(), x, y, z, 0);
        if (pFX)
        {
            pFX->xvel = actor->xvel + Random2(-dx);
            pFX->yvel = actor->yvel + Random2(-dy);
            pFX->zvel = actor->zvel - Random(0x1aaaa);
        }
    }
    if (pXSprite->burnTime > 0)
        evPostActor(actor, 5, kCallbackFXFlameLick);
}

void Remove(DBloodActor* actor, sectortype*) // 1
{
    if (!actor) return;
    evKillActor(actor, kCallbackFXFlareSpark);
    if (actor->hasX())
        seqKill(actor);
    sfxKill3DSound(actor, 0, -1);
    DeleteSprite(actor);
}

void FlareBurst(DBloodActor* actor, sectortype*) // 2
{
    if (!actor) return;
    int nAngle = getangle(actor->xvel, actor->yvel);
    int nRadius = 0x55555;
    for (int i = 0; i < 8; i++)
    {
        auto spawnedactor = actSpawnSprite(actor, 5);
        spawnedactor->spr.picnum = 2424;
        spawnedactor->spr.shade = -128;
        spawnedactor->spr.xrepeat = spawnedactor->spr.yrepeat = 32;
        spawnedactor->spr.type = kMissileFlareAlt;
        spawnedactor->spr.clipdist = 2;
        spawnedactor->SetOwner(actor);
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
        spawnedactor->xvel += dx;
        spawnedactor->yvel += dy;
        spawnedactor->zvel += dz;
        evPostActor(spawnedactor, 960, kCallbackRemove);
    }
    evPostActor(actor, 0, kCallbackRemove);
}

void fxFlareSpark(DBloodActor* actor, sectortype*) // 3
{
    if (!actor) return;
    auto pFX = gFX.fxSpawnActor(FX_28, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->xvel = actor->xvel + Random2(0x1aaaa);
        pFX->yvel = actor->yvel + Random2(0x1aaaa);
        pFX->zvel = actor->zvel - Random(0x1aaaa);
    }
    evPostActor(actor, 4, kCallbackFXFlareSpark);
}

void fxFlareSparkLite(DBloodActor* actor, sectortype*) // 4
{
    if (!actor) return;
    auto pFX = gFX.fxSpawnActor(FX_28, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->xvel = actor->xvel + Random2(0x1aaaa);
        pFX->yvel = actor->yvel + Random2(0x1aaaa);
        pFX->zvel = actor->zvel - Random(0x1aaaa);
    }
    evPostActor(actor, 12, kCallbackFXFlareSparkLite);
}

void fxZombieBloodSpurt(DBloodActor* actor, sectortype*) // 5
{
    if (!actor) return;
    assert(actor->hasX());
    XSPRITE *pXSprite = &actor->x();
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    auto pFX = gFX.fxSpawnActor(FX_27, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, top, 0);
    if (pFX)
    {
        pFX->xvel = actor->xvel + Random2(0x11111);
        pFX->yvel = actor->yvel + Random2(0x11111);
        pFX->zvel = actor->zvel - 0x6aaaa;
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

void fxBloodSpurt(DBloodActor* actor, sectortype*) // 6
{
    if (!actor) return;
    auto pFX = gFX.fxSpawnActor(FX_27, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->spr.ang = 0;
        pFX->xvel = actor->xvel>>8;
        pFX->yvel = actor->yvel>>8;
        pFX->zvel = actor->zvel>>8;
    }
    evPostActor(actor, 6, kCallbackFXBloodSpurt);
}


void fxArcSpark(DBloodActor* actor, sectortype*) // 7
{
    if (!actor) return;
    auto pFX = gFX.fxSpawnActor(FX_15, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->xvel = actor->xvel + Random2(0x10000);
        pFX->yvel = actor->yvel + Random2(0x10000);
        pFX->zvel = actor->zvel - Random(0x1aaaa);
    }
    evPostActor(actor, 3, kCallbackFXArcSpark);
}


void fxDynPuff(DBloodActor* actor, sectortype*) // 8
{
    if (!actor) return;
    if (actor->zvel)
    {
        int nDist = (actor->spr.xrepeat*(tileWidth(actor->spr.picnum)/2))>>2;
        int x = actor->spr.pos.X + MulScale(nDist, Cos(actor->spr.ang-512), 30);
        int y = actor->spr.pos.Y + MulScale(nDist, Sin(actor->spr.ang-512), 30);
        int z = actor->spr.pos.Z;
        auto pFX = gFX.fxSpawnActor(FX_7, actor->spr.sector(), x, y, z, 0);
        if (pFX)
        {
            pFX->xvel = actor->xvel;
            pFX->yvel = actor->yvel;
            pFX->zvel = actor->zvel;
        }
    }
    evPostActor(actor, 12, kCallbackFXDynPuff);
}

void Respawn(DBloodActor* actor, sectortype*) // 9
{
    if (!actor) return;
    assert(actor->hasX());
    XSPRITE *pXSprite = &actor->x();
    
    if (actor->spr.statnum != kStatRespawn && actor->spr.statnum != kStatThing) {
        viewSetSystemMessage("Sprite #%d is not on Respawn or Thing list\n", actor->GetIndex());
        return;
    } else if (!(actor->spr.flags & kHitagRespawn)) {
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
            assert(actor->spr.owner != kStatRespawn);
            assert(actor->spr.owner >= 0 && actor->spr.owner < kMaxStatus);
            ChangeActorStat(actor, actor->spr.owner);
            actor->spr.type = actor->spr.inittype;
            actor->SetOwner(nullptr);
            actor->spr.flags &= ~kHitagRespawn;
            actor->xvel = actor->yvel = actor->zvel = 0;
            pXSprite->respawnPending = 0;
            pXSprite->burnTime = 0;
            pXSprite->isTriggered = 0;
            if (actor->IsDudeActor()) 
            {
                int nType = actor->spr.type-kDudeBase;
                actor->spr.pos = actor->basePoint;
                actor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
                #ifdef NOONE_EXTENSIONS
                if (!gModernMap || pXSprite->sysData2 <= 0) pXSprite->health = dudeInfo[actor->spr.type - kDudeBase].startHealth << 4;
                else pXSprite->health = ClipRange(pXSprite->sysData2 << 4, 1, 65535);

                switch (actor->spr.type) {
                    default:
                        actor->spr.clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
                        if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
                            seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor, -1);
                        break;
                    case kDudeModernCustom:
                        seqSpawn(genDudeSeqStartId(actor), actor, -1);
                        break;
                }
                
                // return dude to the patrol state
                if (gModernMap && pXSprite->dudeFlag4) {
                    pXSprite->data3 = 0;
                    actor->SetTarget(nullptr);
                }
                #else
                actor->spr.clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
                pXSprite->health = getDudeInfo(nType + kDudeBase)->startHealth << 4;
                if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
                    seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor, -1);
                #endif
                aiInitSprite(actor);
                pXSprite->key = 0;
            } else if (actor->spr.type == kThingTNTBarrel) {
                actor->spr.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
                actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
            }

            gFX.fxSpawnActor(FX_29, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
            sfxPlay3DSound(actor, 350, -1, 0);
            break;
        }
    }
}

void PlayerBubble(DBloodActor* actor, sectortype*) // 10
{
    if (!actor) return;
    if (actor->IsPlayerActor())
    {
        PLAYER *pPlayer = &gPlayer[actor->spr.type-kDudePlayer1];
        if (!pPlayer->bubbleTime)
            return;
        int top, bottom;
        GetActorExtents(actor, &top, &bottom);
        for (int i = 0; i < (pPlayer->bubbleTime>>6); i++)
        {
            int nDist = (actor->spr.xrepeat*(tileWidth(actor->spr.picnum)/2))>>2;
            int nAngle = Random(2048);
            int x = actor->spr.pos.X + MulScale(nDist, Cos(nAngle), 30);
            int y = actor->spr.pos.Y + MulScale(nDist, Sin(nAngle), 30);
            int z = bottom-Random(bottom-top);
            auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23+Random(3)), actor->spr.sector(), x, y, z, 0);
            if (pFX)
            {
                pFX->xvel = actor->xvel + Random2(0x1aaaa);
                pFX->yvel = actor->yvel + Random2(0x1aaaa);
                pFX->zvel = actor->zvel + Random2(0x1aaaa);
            }
        }
        evPostActor(actor, 4, kCallbackPlayerBubble);
    }
}

void EnemyBubble(DBloodActor* actor, sectortype*) // 11
{
    if (!actor) return;
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    for (int i = 0; i < (abs(actor->zvel)>>18); i++)
    {
        int nDist = (actor->spr.xrepeat*(tileWidth(actor->spr.picnum)/2))>>2;
        int nAngle = Random(2048);
        int x = actor->spr.pos.X + MulScale(nDist, Cos(nAngle), 30);
        int y = actor->spr.pos.Y + MulScale(nDist, Sin(nAngle), 30);
        int z = bottom-Random(bottom-top);
        auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23+Random(3)), actor->spr.sector(), x, y, z, 0);
        if (pFX)
        {
            pFX->xvel = actor->xvel + Random2(0x1aaaa);
            pFX->yvel = actor->yvel + Random2(0x1aaaa);
            pFX->zvel = actor->zvel + Random2(0x1aaaa);
        }
    }
    evPostActor(actor, 4, kCallbackEnemeyBubble);
}

void CounterCheck(DBloodActor*, sectortype* pSector) // 12
{
    if (!pSector || pSector->type != kSectorCounter) return;
    if (!pSector->hasX()) return;
    
    XSECTOR* pXSector = &pSector->xs();
    int nReq = pXSector->waitTimeA;
    int nType = pXSector->data;
    int nCount = 0;
    if (!nType || !nReq) return;
    
    BloodSectIterator it(pSector);
    while (auto actor = it.Next())
    {
        if (actor->spr.type == nType) nCount++;
    }
        
    if (nCount < nReq) {
        evPostSector(pSector, 5, kCallbackCounterCheck);
        return;
    } else {
        //pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
        trTriggerSector(pSector, kCmdOn);
        pXSector->locked = 1; //lock sector, so it can be opened again later
    }
}


void FinishHim(DBloodActor* actor, sectortype*) // 13
{
    if (!actor) return;
    if (actor->IsPlayerActor() && playerSeqPlaying(&gPlayer[actor->spr.type - kDudePlayer1], 16) && actor == gMe->actor)
        sndStartSample(3313, -1, 1, 0);
}

void fxBloodBits(DBloodActor* actor, sectortype*) // 14
{
    if (!actor) return;
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;
    GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    actor->spr.pos.Z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = actor->spr.pos.X+MulScale(nDist, Cos(nAngle), 28);
    int y = actor->spr.pos.Y+MulScale(nDist, Sin(nAngle), 28);
    gFX.fxSpawnActor(FX_48, actor->spr.sector(), x, y, actor->spr.pos.Z, 0);
    if (actor->spr.ang == 1024)
    {
        int nChannel = 28 + (actor->GetIndex() & 2);    // this is a little stupid...
        sfxPlay3DSound(actor, 385, nChannel, 1);
    }
    if (Chance(0x5000))
    {
        auto pFX = gFX.fxSpawnActor(FX_36, actor->spr.sector(), x, y, floorZ-64, 0);
        if (pFX)
            pFX->spr.ang = nAngle;
    }
    gFX.remove(actor);
}


void fxTeslaAlt(DBloodActor* actor, sectortype*) // 15
{
    if (!actor) return;
    auto pFX = gFX.fxSpawnActor(FX_49, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->xvel = actor->xvel + Random2(0x1aaaa);
        pFX->yvel = actor->yvel + Random2(0x1aaaa);
        pFX->zvel = actor->zvel - Random(0x1aaaa);
    }
    evPostActor(actor, 3, kCallbackFXTeslaAlt);
}


int tommySleeveSnd[] = { 608, 609, 611 }; // unused?
int sawedOffSleeveSnd[] = { 610, 612 };

void fxBouncingSleeve(DBloodActor* actor, sectortype*) // 16
{
    if (!actor) return;
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;

    GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
    int top, bottom; GetActorExtents(actor, &top, &bottom);
    actor->spr.pos.Z += floorZ - bottom;
    
    int zv = actor->zvel - actor->spr.sector()->velFloor;
    
    if (actor->zvel == 0) sleeveStopBouncing(actor);
    else if (zv > 0) {
        actFloorBounceVector((int*)& actor->xvel, (int*)& actor->yvel, &zv, actor->spr.sector(), 0x9000);
        actor->zvel = zv;
        if (actor->spr.sector()->velFloor == 0 && abs(actor->zvel) < 0x20000)  {
            sleeveStopBouncing(actor);
            return;
        }

        int nChannel = 28 + (actor->GetIndex() & 2);
        
        // tommy sleeve
        if (actor->spr.type >= 37 && actor->spr.type <= 39) {
            Random(3); 
            sfxPlay3DSound(actor, 608 + Random(2), nChannel, 1);
        
        // sawed-off sleeve
        } else {
            sfxPlay3DSound(actor, sawedOffSleeveSnd[Random(2)], nChannel, 1);
        }
    }   

}


void sleeveStopBouncing(DBloodActor* actor) 
{
    actor->xvel = actor->yvel = actor->zvel = 0;
    if (actor->hasX()) seqKill(actor);
    sfxKill3DSound(actor, -1, -1);

    switch (actor->spr.type) {
    case 37:
    case 38:
    case 39:
        actor->spr.picnum = 2465;
        break;
    case 40:
    case 41:
    case 42:
        actor->spr.picnum = 2464;
        break;
    }

    actor->spr.type = 51;
    actor->spr.xrepeat = actor->spr.yrepeat = 10;
}


void returnFlagToBase(DBloodActor* actor, sectortype*) // 17
{
    if (!actor) return;
    auto aOwner = actor->GetOwner();
    if (aOwner)
    {
        switch (actor->spr.type) 
        {
            case kItemFlagA:
                trTriggerSprite(aOwner, kCmdOn);
                sndStartSample(8003, 255, 2, 0);
                gBlueFlagDropped = false;
                viewSetMessage("Blue Flag returned to base.");
                break;
            case kItemFlagB:
                trTriggerSprite(aOwner, kCmdOn);
                sndStartSample(8002, 255, 2, 0);
                gRedFlagDropped = false;
                viewSetMessage("Red Flag returned to base.");
                break;
        }
    }
    evPostActor(actor, 0, kCallbackRemove);
}

void fxPodBloodSpray(DBloodActor* actor, sectortype*) // 18
{
    if (!actor) return;
    DBloodActor* pFX;
    if (actor->spr.type == 53)
        pFX = gFX.fxSpawnActor(FX_53, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    else
        pFX = gFX.fxSpawnActor(FX_54, actor->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, 0);
    if (pFX)
    {
        pFX->spr.ang = 0;
        pFX->xvel = actor->xvel >> 8;
        pFX->yvel = actor->yvel >> 8;
        pFX->zvel = actor->zvel >> 8;
    }
    evPostActor(actor, 6, kCallbackFXPodBloodSpray);
}

void fxPodBloodSplat(DBloodActor* actor, sectortype*) // 19
{
    if (!actor) return;
    int ceilZ, floorZ;
    Collision floorColl, ceilColl;

    GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    actor->spr.pos.Z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = actor->spr.pos.X+MulScale(nDist, Cos(nAngle), 28);
    int y = actor->spr.pos.Y+MulScale(nDist, Sin(nAngle), 28);
    if (actor->spr.ang == 1024)
    {
        int nChannel = 28 + (actor->GetIndex() & 2);
        assert(nChannel < 32);
        sfxPlay3DSound(actor, 385, nChannel, 1);
    }
    DBloodActor *pFX = NULL;
    if (actor->spr.type == 53 || actor->spr.type == kThingPodGreenBall)
    {
        if (Chance(0x500) || actor->spr.type == kThingPodGreenBall)
            pFX = gFX.fxSpawnActor(FX_55, actor->spr.sector(), x, y, floorZ-64, 0);
        if (pFX)
            pFX->spr.ang = nAngle;
    }
    else
    {
        pFX = gFX.fxSpawnActor(FX_32, actor->spr.sector(), x, y, floorZ-64, 0);
        if (pFX)
            pFX->spr.ang = nAngle;
    }
    gFX.remove(actor);
}



void LeechStateTimer(DBloodActor* actor, sectortype*) // 20
{
    if (!actor) return;
    if (actor->spr.statnum == kStatThing && !(actor->spr.flags & 32)) {
        switch (actor->spr.type) {
            case kThingDroppedLifeLeech:
            #ifdef NOONE_EXTENSIONS
            case kModernThingEnemyLifeLeech:
            #endif
                actor->xspr.stateTimer = 0;
                break;
        }
    }
}

void sub_76A08(DBloodActor *actor, DBloodActor *actor2, PLAYER *pPlayer) // ???
{
    int top, bottom;
    GetActorExtents(actor, &top, &bottom);
    actor->spr.pos.X = actor2->spr.pos.X;
    actor->spr.pos.Y = actor2->spr.pos.Y;
    actor->spr.pos.Z = actor2->spr.sector()->floorz-(bottom-actor->spr.pos.Z);
    actor->spr.ang = actor2->spr.ang;
    ChangeActorSect(actor, actor2->spr.sector());
    sfxPlay3DSound(actor2, 201, -1, 0);
    actor->xvel = actor->yvel = actor->zvel = 0;
    viewBackupSpriteLoc(actor);
    if (pPlayer)
    {
        playerResetInertia(pPlayer);
        pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
    }
}

void DropVoodooCb(DBloodActor* actor, sectortype*) // unused
{
    if (!actor) return;
    auto Owner = actor->GetOwner();
    if (Owner == nullptr)
    {
        evPostActor(actor, 0, kCallbackRemove);
        return;
    }
    PLAYER *pPlayer;
    if (Owner->IsPlayerActor())
        pPlayer = &gPlayer[Owner->spr.type-kDudePlayer1];
    else
        pPlayer = nullptr;
    if (!pPlayer)
    {
        evPostActor(actor, 0, kCallbackRemove);
        return;
    }
    actor->spr.ang = getangle(Owner->spr.pos.X-actor->spr.pos.X, Owner->spr.pos.Y-actor->spr.pos.Y);
    if (actor->hasX())
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
            if (actor2->hasX())
            {
                XSPRITE *pXSprite2 = &actor2->x();
                PLAYER *pPlayer2;
                if (actor2->IsPlayerActor())
                    pPlayer2 = &gPlayer[actor2->spr.type-kDudePlayer1];
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
                            sub_76A08(actor2, actor, pPlayer2);
                            evPostActor(actor, 0, kCallbackRemove);
                            return;
                        }
                    }
                    else
                    {
                        int vd = 0x2666;
                        switch (actor2->spr.type)
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
                            sub_76A08(actor2, actor, NULL);
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

void callbackCondition(DBloodActor* actor, sectortype*)
{
    XSPRITE* pXSprite = &actor->x();
    if (pXSprite->isTriggered) return;

    TRCONDITION const* pCond = &gCondition[pXSprite->sysData1];
    for (unsigned i = 0; i < pCond->length; i++) {
        EVENT evn;  
        evn.target = pCond->obj[i].obj;
        evn.cmd = pCond->obj[i].cmd; 
        evn.funcID = kCallbackCondition;
        useCondition(actor, evn);
    }

    evPostActor(actor, pXSprite->busyTime, kCallbackCondition);
    return;
}

void(*gCallback[kCallbackMax])(DBloodActor*, sectortype*) =
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
