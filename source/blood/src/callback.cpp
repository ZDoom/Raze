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

#include "build.h"
#include "common_game.h"

#include "actor.h"
#include "ai.h"
#include "blood.h"
#include "callback.h"
#include "config.h"
#include "db.h"
#include "dude.h"
#include "eventq.h"
#include "fx.h"
#include "gameutil.h"
#include "globals.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "trig.h"
#include "triggers.h"
#include "view.h"


void sub_74C20(int nSprite) // 7
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_15, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x10000);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x10000);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 3, CALLBACK_ID_7);
}

void sub_74D04(int nSprite) // 15
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_49, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 3, CALLBACK_ID_15);
}

void FinishHim(int nSprite) // 13
{
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    if (playerSeqPlaying(&gPlayer[pSprite->type-kDudePlayer1], 16) && pXSprite->target == gMe->at5b)
        sndStartSample(3313, -1, 1, 0);
}

void FlameLick(int nSprite) // 0
{
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < 3; i++)
    {
        int nDist = (pSprite->xrepeat*(tilesiz[pSprite->picnum].x/2))>>3;
        int nAngle = Random(2048);
        int dx = mulscale30(nDist, Cos(nAngle));
        int dy = mulscale30(nDist, Sin(nAngle));
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
        evPost(nSprite, 3, 5, CALLBACK_ID_0);
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
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;
    for (int i = 0; i < 8; i++)
    {
        spritetype *pSpawn = actSpawnSprite(pSprite, 5);
        pSpawn->picnum = 2424;
        pSpawn->shade = -128;
        pSpawn->xrepeat = pSpawn->yrepeat = 32;
        pSpawn->type = 303;
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
        evPost(pSpawn->index, 3, 960, CALLBACK_ID_1);
    }
    evPost(nSprite, 3, 0, CALLBACK_ID_1);
}

void FlareSpark(int nSprite) // 3
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 4, CALLBACK_ID_3);
}

void FlareSparkLite(int nSprite) // 4
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_28, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
        yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
        zvel[pFX->index] = zvel[nSprite] - Random(0x1aaaa);
    }
    evPost(nSprite, 3, 12, CALLBACK_ID_4);
}

void ZombieSpurt(int nSprite) // 5
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
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
        evPost(nSprite, 3, 4, CALLBACK_ID_5);
        pXSprite->data1 -= 4;
    }
    else if (pXSprite->data2 > 0)
    {
        evPost(nSprite, 3, 60, CALLBACK_ID_5);
        pXSprite->data1 = 40;
        pXSprite->data2--;
    }
}

void BloodSpurt(int nSprite) // 6
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
    evPost(nSprite, 3, 6, CALLBACK_ID_6);
}

void DynPuff(int nSprite) // 8
{
    spritetype *pSprite = &sprite[nSprite];
    if (zvel[nSprite])
    {
        int nDist = (pSprite->xrepeat*(tilesiz[pSprite->picnum].x/2))>>2;
        int x = pSprite->x + mulscale30(nDist, Cos(pSprite->ang-512));
        int y = pSprite->y + mulscale30(nDist, Sin(pSprite->ang-512));
        int z = pSprite->z;
        spritetype *pFX = gFX.fxSpawn(FX_7, pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            xvel[pFX->index] = xvel[nSprite];
            yvel[pFX->index] = yvel[nSprite];
            zvel[pFX->index] = zvel[nSprite];
        }
    }
    evPost(nSprite, 3, 12, CALLBACK_ID_8);
}

void Respawn(int nSprite) // 9
{
    spritetype *pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    if (pSprite->statnum != 8 && pSprite->statnum != 4)
        ThrowError("Sprite %d is not on Respawn or Thing list\n", nSprite);
    if (!(pSprite->hitag&16))
        ThrowError("Sprite %d does not have the respawn attribute\n", nSprite);
    switch (pXSprite->respawnPending)
    {
    case 1:
    {
        int nTime = mulscale16(actGetRespawnTime(pSprite), 0x4000);
        pXSprite->respawnPending = 2;
        evPost(nSprite, 3, nTime, CALLBACK_ID_9);
        break;
    }
    case 2:
    {
        int nTime = mulscale16(actGetRespawnTime(pSprite), 0x2000);
        pXSprite->respawnPending = 3;
        evPost(nSprite, 3, nTime, CALLBACK_ID_9);
        break;
    }
    case 3:
    {
        dassert(pSprite->owner != kStatRespawn);
        dassert(pSprite->owner >= 0 && pSprite->owner < kMaxStatus);
        ChangeSpriteStat(nSprite, pSprite->owner);
        pSprite->type = pSprite->zvel;
        pSprite->owner = -1;
        pSprite->hitag &= ~16;
        xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
        pXSprite->respawnPending = 0;
        pXSprite->burnTime = 0;
        pXSprite->isTriggered = 0;
        if (pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
        {
            int nType = pSprite->type-kDudeBase;
            pSprite->x = baseSprite[nSprite].x;
            pSprite->y = baseSprite[nSprite].y;
            pSprite->z = baseSprite[nSprite].z;
            pSprite->cstat |= 0x1101;
            pSprite->clipdist = dudeInfo[nType].clipdist;
            pXSprite->health = dudeInfo[nType].startHealth<<4;
            if (gSysRes.Lookup(dudeInfo[nType].seqStartID, "SEQ"))
                seqSpawn(dudeInfo[nType].seqStartID, 3, pSprite->extra, -1);
            aiInitSprite(pSprite);
            pXSprite->key = 0;
        }
        if (pSprite->type == 400)
        {
            pSprite->cstat |= 257;
            pSprite->cstat &= (unsigned short)~32768;
        }
        gFX.fxSpawn(FX_29, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
        sfxPlay3DSound(pSprite, 350, -1, 0);
        break;
    }
    default:
        ThrowError("Unexpected respawnPending value = %d", pXSprite->respawnPending);
        break;
    }
}

void PlayerBubble(int nSprite) // 10
{
    spritetype *pSprite = &sprite[nSprite];
    if (IsPlayerSprite(pSprite))
    {
        PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
        dassert(pPlayer != NULL);
        if (!pPlayer->at302)
            return;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        for (int i = 0; i < (pPlayer->at302>>6); i++)
        {
            int nDist = (pSprite->xrepeat*(tilesiz[pSprite->picnum].x/2))>>2;
            int nAngle = Random(2048);
            int x = pSprite->x + mulscale30(nDist, Cos(nAngle));
            int y = pSprite->y + mulscale30(nDist, Sin(nAngle));
            int z = bottom-Random(bottom-top);
            spritetype *pFX = gFX.fxSpawn((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
            if (pFX)
            {
                xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
                yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
                zvel[pFX->index] = zvel[nSprite] + Random2(0x1aaaa);
            }
        }
        evPost(nSprite, 3, 4, CALLBACK_ID_10);
    }
}

void EnemyBubble(int nSprite) // 11
{
    spritetype *pSprite = &sprite[nSprite];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < (klabs(zvel[nSprite])>>18); i++)
    {
        int nDist = (pSprite->xrepeat*(tilesiz[pSprite->picnum].x/2))>>2;
        int nAngle = Random(2048);
        int x = pSprite->x + mulscale30(nDist, Cos(nAngle));
        int y = pSprite->y + mulscale30(nDist, Sin(nAngle));
        int z = bottom-Random(bottom-top);
        spritetype *pFX = gFX.fxSpawn((FX_ID)(FX_23+Random(3)), pSprite->sectnum, x, y, z, 0);
        if (pFX)
        {
            xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
            yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
            zvel[pFX->index] = zvel[nSprite] + Random2(0x1aaaa);
        }
    }
    evPost(nSprite, 3, 4, CALLBACK_ID_11);
}

void CounterCheck(int nSector) // 12
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    // By NoOne: edits for counter sector new features.
    // remove check below, so every sector can be counter if command 12 (this callback) received.
    //if (pSector->lotag != 619) return;
    int nXSprite = pSector->extra;
    if (nXSprite > 0)
    {
        XSECTOR *pXSector = &xsector[nXSprite];
        int nReq = pXSector->waitTimeA;
        int nType = pXSector->data;
        if (nType && nReq)
        {
            int nCount = 0;
            for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
            {
                if (sprite[nSprite].type == nType)
                    nCount++;
            }
            if (nCount >= nReq)
            {

                //pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
                trTriggerSector(nSector, pXSector, 1);
                pXSector->locked = 1; //lock sector, so it can be opened again later
            }
            else
                evPost(nSector, 6, 5, CALLBACK_ID_12);
        }
    }
}

void sub_76140(int nSprite) // 14
{
    spritetype *pSprite = &sprite[nSprite];
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = pSprite->x+mulscale28(nDist, Cos(nAngle));
    int y = pSprite->y+mulscale28(nDist, Sin(nAngle));
    gFX.fxSpawn(FX_48, pSprite->sectnum, x, y, pSprite->z, 0);
    if (pSprite->ang == 1024)
    {
        int nChannel = 28+(pSprite->index&2);
        dassert(nChannel < 32);
        sfxPlay3DSound(pSprite, 385, nChannel, 1);
    }
    if (Chance(0x5000))
    {
        spritetype *pFX = gFX.fxSpawn(FX_36, pSprite->sectnum, x, y, floorZ-64, 0);
        if (pFX)
            pFX->ang = nAngle;
    }
    gFX.sub_73FFC(nSprite);
}

void sub_7632C(spritetype *pSprite)
{
    xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    sfxKill3DSound(pSprite, -1, -1);
    switch (pSprite->type)
    {
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

int dword_13B32C[] = { 608, 609, 611 };
int dword_13B338[] = { 610, 612 };

void sub_763BC(int nSprite) // 16
{
    spritetype *pSprite = &sprite[nSprite];
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int zv = zvel[nSprite]-velFloor[pSprite->sectnum];
    if (zv > 0)
    {
        actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], &zv, pSprite->sectnum, 0x9000);
        zvel[nSprite] = zv;
        if (velFloor[pSprite->sectnum] == 0 && klabs(zvel[nSprite]) < 0x20000)
        {
            sub_7632C(pSprite);
            return;
        }
        int nChannel = 28+(pSprite->index&2);
        dassert(nChannel < 32);
        if (pSprite->type >= 37 && pSprite->type <= 39)
        {
            Random(3);
            sfxPlay3DSound(pSprite, 608+Random(2), nChannel, 1);
        }
        else
            sfxPlay3DSound(pSprite, dword_13B338[Random(2)], nChannel, 1);
    }
    else if (zvel[nSprite] == 0)
        sub_7632C(pSprite);
}

void sub_765B8(int nSprite) // 17
{
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->owner >= 0 && pSprite->owner < kMaxSprites)
    {
        spritetype *pOwner = &sprite[pSprite->owner];
        XSPRITE *pXOwner = &xsprite[pOwner->extra];
        switch (pSprite->type)
        {
        case 147:
            trTriggerSprite(pOwner->index, pXOwner, 1);
            sndStartSample(8003, 255, 2, 0);
            viewSetMessage("Blue Flag returned to base.");
            break;
        case 148:
            trTriggerSprite(pOwner->index, pXOwner, 1);
            sndStartSample(8002, 255, 2, 0);
            viewSetMessage("Red Flag returned to base.");
            break;
        }
    }
    evPost(pSprite->index, 3, 0, CALLBACK_ID_1);
}

void sub_766B8(int nSprite) // 19
{
    spritetype *pSprite = &sprite[nSprite];
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist, CLIPMASK0);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += floorZ-bottom;
    int nAngle = Random(2048);
    int nDist = Random(16)<<4;
    int x = pSprite->x+mulscale28(nDist, Cos(nAngle));
    int y = pSprite->y+mulscale28(nDist, Sin(nAngle));
    if (pSprite->ang == 1024)
    {
        int nChannel = 28+(pSprite->index&2);
        dassert(nChannel < 32);
        sfxPlay3DSound(pSprite, 385, nChannel, 1);
    }
    spritetype *pFX = NULL;
    if (pSprite->type == 53 || pSprite->type == 430)
    {
        if (Chance(0x500) || pSprite->type == 430)
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
    gFX.sub_73FFC(nSprite);
}

void sub_768E8(int nSprite) // 18
{
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX;
    if (pSprite->type == 53)
        pFX = gFX.fxSpawn(FX_53, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    else
        pFX = gFX.fxSpawn(FX_54, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        pFX->ang = 0;
        xvel[pFX->index] = xvel[nSprite]>>8;
        yvel[pFX->index] = yvel[nSprite]>>8;
        zvel[pFX->index] = zvel[nSprite]>>8;
    }
    evPost(nSprite, 3, 6, CALLBACK_ID_18);
}

void sub_769B4(int nSprite) // 19
{
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->statnum == 4 && !(pSprite->hitag & 32)) {
        switch (pSprite->lotag) {
            case 431:
            case kGDXThingCustomDudeLifeLeech:
                xsprite[pSprite->extra].stateTimer = 0;
                break;
        }
    }
}

void sub_76A08(spritetype *pSprite, spritetype *pSprite2, PLAYER *pPlayer)
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
        pPlayer->at6b = pPlayer->at73 = 0;
    }
}

void sub_76B78(int nSprite)
{
    spritetype *pSprite = &sprite[nSprite];
    int nOwner = actSpriteOwnerToSpriteId(pSprite);
    if (nOwner < 0 || nOwner >= kMaxSprites)
    {
        evPost(nSprite, 3, 0, CALLBACK_ID_1);
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
        evPost(nSprite, 3, 0, CALLBACK_ID_1);
        return;
    }
    pSprite->ang = getangle(pOwner->x-pSprite->x, pOwner->y-pSprite->y);
    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE *pXSprite = &xsprite[nXSprite];
        if (pXSprite->data1 == 0)
        {
            evPost(nSprite, 3, 0, CALLBACK_ID_1);
            return;
        }
        int nSprite2, nNextSprite;
        for (nSprite2 = headspritestat[6]; nSprite2 >= 0; nSprite2 = nNextSprite)
        {
            nNextSprite = nextspritestat[nSprite2];
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
                        if (gGameOptions.nGameType == 3 && pPlayer->at2ea == pPlayer2->at2ea)
                            continue;
                        int t = 0x8000/ClipLow(gNetPlayers-1, 1);
                        if (!powerupCheck(pPlayer2, 14))
                            t += ((3200-pPlayer2->at33e[2])<<15)/3200;
                        if (Chance(t) || nNextSprite < 0)
                        {
                            int nDmg = actDamageSprite(nOwner, pSprite2, DAMAGE_TYPE_5, pXSprite->data1<<4);
                            pXSprite->data1 = ClipLow(pXSprite->data1-nDmg, 0);
                            sub_76A08(pSprite2, pSprite, pPlayer2);
                            evPost(nSprite, 3, 0, CALLBACK_ID_1);
                            return;
                        }
                    }
                    else
                    {
                        int vd = 0x2666;
                        switch (pSprite2->type)
                        {
                        case 218:
                        case 219:
                        case 220:
                        case 250:
                        case 251:
                            vd = 0x147;
                            break;
                        case 205:
                        case 221:
                        case 222:
                        case 223:
                        case 224:
                        case 225:
                        case 226:
                        case 227:
                        case 228:
                        case 229:
                        case 239:
                        case 240:
                        case 241:
                        case 242:
                        case 243:
                        case 244:
                        case 245:
                        case 252:
                        case 253:
                            vd = 0;
                            break;
                        }
                        if (vd && (Chance(vd) || nNextSprite < 0))
                        {
                            sub_76A08(pSprite2, pSprite, NULL);
                            evPost(nSprite, 3, 0, CALLBACK_ID_1);
                            return;
                        }
                    }
                }
            }
        }
        pXSprite->data1 = ClipLow(pXSprite->data1-1, 0);
        evPost(nSprite, 3, 0, CALLBACK_ID_1);
    }
}

void(*gCallback[kCallbackMax])(int) =
{
    FlameLick,
    Remove,
    FlareBurst,
    FlareSpark,
    FlareSparkLite,
    ZombieSpurt,
    BloodSpurt,
    sub_74C20,
    DynPuff,
    Respawn,
    PlayerBubble,
    EnemyBubble,
    CounterCheck,
    FinishHim,
    sub_76140,
    sub_74D04,
    sub_763BC,
    sub_765B8,
    sub_768E8,
    sub_766B8,
    sub_769B4,
    sub_76B78
};
