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
#include <random>
#include <iostream>

#include "build.h"
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"

#include "ai.h"
#include "actor.h"
#include "blood.h"
#include "db.h"
#include "endgame.h"
#include "eventq.h"

#include "aiunicult.h"
#include "fx.h"
#include "gameutil.h"
#include "gib.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "triggers.h"
#include "trig.h"
#include "view.h"

int basePath[kMaxSectors];

void FireballTrapSeqCallback(int, int);
void UniMissileTrapSeqCallback(int, int);
void MGunFireSeqCallback(int, int);
void MGunOpenSeqCallback(int, int);

int nFireballTrapClient = seqRegisterClient(FireballTrapSeqCallback);
int nUniMissileTrapClient = seqRegisterClient(UniMissileTrapSeqCallback);
int nMGunFireClient = seqRegisterClient(MGunFireSeqCallback);
int nMGunOpenClient = seqRegisterClient(MGunOpenSeqCallback);

unsigned int GetWaveValue(unsigned int nPhase, int nType)
{
    switch (nType)
    {
    case 0:
        return 0x8000-(Cos((nPhase<<10)>>16)>>15);
    case 1:
        return nPhase;
    case 2:
        return 0x10000-(Cos((nPhase<<9)>>16)>>14);
    case 3:
        return Sin((nPhase<<9)>>16)>>14;
    }
    return nPhase;
}

char SetSpriteState(int nSprite, XSPRITE *pXSprite, int nState)
{
    if ((pXSprite->busy&0xffff) == 0 && pXSprite->state == nState)
        return 0;
    pXSprite->busy = nState<<16;
    pXSprite->state = nState;
    evKill(nSprite, 3);
    if ((sprite[nSprite].hitag & 16) != 0 && sprite[nSprite].type >= kDudeBase && sprite[nSprite].type < kDudeMax)
    {
        pXSprite->respawnPending = 3;
        evPost(nSprite, 3, gGameOptions.nMonsterRespawnTime, CALLBACK_ID_9);
        return 1;
    }
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime*120) / 10, pXSprite->restState ? COMMAND_ID_1 : COMMAND_ID_0);
    if (pXSprite->txID)
    {
        if (pXSprite->command != 5 && pXSprite->triggerOn && pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
        if (pXSprite->command != 5 && pXSprite->triggerOff && !pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
    }
    return 1;
}

char SetWallState(int nWall, XWALL *pXWall, int nState)
{
    if ((pXWall->busy&0xffff) == 0 && pXWall->state == nState)
        return 0;
    pXWall->busy = nState<<16;
    pXWall->state = nState;
    evKill(nWall, 0);
    if (pXWall->restState != nState && pXWall->waitTime > 0)
        evPost(nWall, 0, (pXWall->waitTime*120) / 10, pXWall->restState ? COMMAND_ID_1 : COMMAND_ID_0);
    if (pXWall->txID)
    {
        if (pXWall->command != 5 && pXWall->triggerOn && pXWall->state)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
        if (pXWall->command != 5 && pXWall->triggerOff && !pXWall->state)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
    }
    return 1;
}

char SetSectorState(int nSector, XSECTOR *pXSector, int nState)
{
    if ((pXSector->busy&0xffff) == 0 && pXSector->state == nState)
        return 0;
    pXSector->busy = nState<<16;
    pXSector->state = nState;
    evKill(nSector, 6);
    if (nState == 1)
    {
        if (pXSector->command != 5 && pXSector->triggerOn && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        if (pXSector->at1b_2)
        {
            pXSector->at1b_2 = 0;
            pXSector->at1b_3 = 0;
        }
        else if (pXSector->atf_6)
            evPost(nSector, 6, (pXSector->waitTimeA * 120) / 10, COMMAND_ID_0);
    }
    else
    {
        if (pXSector->command != 5 && pXSector->triggerOff && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        if (pXSector->at1b_3)
        {
            pXSector->at1b_2 = 0;
            pXSector->at1b_3 = 0;
        }
        else if (pXSector->atf_7)
            evPost(nSector, 6, (pXSector->waitTimeB * 120) / 10, COMMAND_ID_1);
    }
    return 1;
}

int gBusyCount = 0;

enum BUSYID {
    BUSYID_0 = 0,
    BUSYID_1,
    BUSYID_2,
    BUSYID_3,
    BUSYID_4,
    BUSYID_5,
    BUSYID_6,
    BUSYID_7,
};

struct BUSY {
    int at0;
    int at4;
    int at8;
    BUSYID atc;
};

BUSY gBusy[128];

void AddBusy(int a1, BUSYID a2, int nDelta)
{
    dassert(nDelta != 0);
    int i;
    for (i = 0; i < gBusyCount; i++)
    {
        if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
            break;
    }
    if (i == gBusyCount)
    {
        if (gBusyCount == 128)
            return;
        gBusy[i].at0 = a1;
        gBusy[i].atc = a2;
        gBusy[i].at8 = nDelta > 0 ? 0 : 65536;
        gBusyCount++;
    }
    gBusy[i].at4 = nDelta;
}

void ReverseBusy(int a1, BUSYID a2)
{
    int i;
    for (i = 0; i < gBusyCount; i++)
    {
        if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
        {
            gBusy[i].at4 = -gBusy[i].at4;
            break;
        }
    }
}

unsigned int GetSourceBusy(EVENT a1)
{
    int nIndex = a1.index;
    switch (a1.type)
    {
    case 6:
    {
        int nXIndex = sector[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
        return xsector[nXIndex].busy;
    }
    case 0:
    {
        int nXIndex = wall[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
        return xwall[nXIndex].busy;
    }
    case 3:
    {
        int nXIndex = sprite[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
        return xsprite[nXIndex].busy;
    }
    }
    return 0;
}

void sub_43CF8(spritetype *pSprite, XSPRITE *pXSprite, EVENT a3)
{
    switch (a3.cmd)
    {
    case 30:
    {
        int nPlayer = pXSprite->data4;
        if (nPlayer >= 0 && nPlayer < gNetPlayers)
        {
            PLAYER *pPlayer = &gPlayer[nPlayer];
            if (pPlayer->pXSprite->health > 0)
            {
                pPlayer->at181[8] = ClipHigh(pPlayer->at181[8]+pXSprite->data3, gAmmoInfo[8].at0);
                pPlayer->atcb[9] = 1;
                if (pPlayer->atbd != 9)
                {
                    pPlayer->atc3 = 0;
                    pPlayer->atbe = 9;
                }
                evKill(pSprite->index, 3);
            }
        }
        break;
    }
    case 35:
    {
        int nTarget = pXSprite->target;
        if (nTarget >= 0 && nTarget < kMaxSprites)
        {
            if (!pXSprite->stateTimer)
            {
                spritetype *pTarget = &sprite[nTarget];
                if (pTarget->statnum == 6 && !(pTarget->hitag&32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites)
                {
                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    int nType = pTarget->type-kDudeBase;
                    DUDEINFO *pDudeInfo = &dudeInfo[nType];
                    int z1 = (top-pSprite->z)-256;
                    int x = pTarget->x;
                    int y = pTarget->y;
                    int z = pTarget->z;
                    int nDist = approxDist(x - pSprite->x, y - pSprite->y);
                    if (nDist != 0 && cansee(pSprite->x, pSprite->y, top, pSprite->sectnum, x, y, z, pTarget->sectnum))
                    {
                        int t = divscale(nDist, 0x1aaaaa, 12);
                        x += (xvel[nTarget]*t)>>12;
                        y += (yvel[nTarget]*t)>>12;
                        int angBak = pSprite->ang;
                        pSprite->ang = getangle(x-pSprite->x, y-pSprite->y);
                        int dx = Cos(pSprite->ang)>>16;
                        int dy = Sin(pSprite->ang)>>16;
                        int tz = pTarget->z - (pTarget->yrepeat * pDudeInfo->aimHeight) * 4;
                        int dz = divscale(tz - top - 256, nDist, 10);
                        int nMissileType = 316+(pXSprite->data3 ? 1 : 0);
                        int t2;
                        if (!pXSprite->data3)
                            t2 = 120 / 10.0;
                        else
                            t2 = (3*120) / 10.0;
                        spritetype *pMissile = actFireMissile(pSprite, 0, z1, dx, dy, dz, nMissileType);
                        if (pMissile)
                        {
                            pMissile->owner = pSprite->owner;
                            pXSprite->stateTimer = 1;
                            evPost(pSprite->index, 3, t2, CALLBACK_ID_20);
                            pXSprite->data3 = ClipLow(pXSprite->data3-1, 0);
                        }
                        pSprite->ang = angBak;
                    }
                }
            }
        }
        return;
    }
    }
    actPostSprite(pSprite->index, kStatFree);
}

void ActivateGenerator(int);

void OperateSprite(int nSprite, XSPRITE *pXSprite, EVENT a3)
{
    spritetype *pSprite = &sprite[nSprite];
    switch (a3.cmd)
    {
    case 6:
        pXSprite->locked = 1;
        switch (pSprite->type) {
        case kGDXWindGenerator:
            stopWindOnSectors(pXSprite);
            break;
        }
        return;
    case 7:
        pXSprite->locked = 0;
        return;
    case 8:
        pXSprite->locked = pXSprite->locked ^ 1;
        switch(pSprite->type) {
         case kGDXWindGenerator:
            if (pXSprite->locked == 1) stopWindOnSectors(pXSprite);
            break;
        }
        return;
    }
    if (pSprite->statnum == 6 && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
    {
        switch (a3.cmd)
        {
        case 0:
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case 35:
            if (pXSprite->state)
                break;
            fallthrough__;
        case 1:
        case 30:
        case 33:
            if (!pXSprite->state)
                SetSpriteState(nSprite, pXSprite, 1);
            aiActivateDude(pSprite, pXSprite);
            break;
        }
        return;
    }
    switch (pSprite->type)
    {
    
    /* - Random Event Switch takes random data field and uses it as TX ID - */
    /* - ranged TX ID is now supported also - */
    case kGDXRandomTX:
    {
        std::default_random_engine rng; int tx = 0;
        // set range of TX ID if data2 and data3 is empty.
        if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

            // data1 must be less than data4
            if (pXSprite->data1 > pXSprite->data4) {
                int tmp = pXSprite->data1;
                pXSprite->data1 = pXSprite->data4;
                pXSprite->data4 = tmp;
            }
            
            int total = pXSprite->data4 - pXSprite->data1;
            int data1 = pXSprite->data1; int result = 0;

            // use true random only for single player mode
            if (gGameOptions.nGameType == 0 && !VanillaMode() && !DemoRecordStatus()) {
                rng.seed(std::random_device()());
                pXSprite->txID = (int)my_random(pXSprite->data1, pXSprite->data4);
            
            // otherwise use Blood's default one. In the future it maybe possible to make
            // host send info to clients about what was generated.
            } else {
                pXSprite->txID = Random(total) + data1;
            }

        } else if ((tx = GetRandDataVal(NULL,pSprite)) > 0) { 
            pXSprite->txID = tx; 
        }

        switch (a3.cmd)
        {
        case COMMAND_ID_0:
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case COMMAND_ID_1:
            SetSpriteState(nSprite, pXSprite, 1);
            break;
        default:
            SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            break;
        }

        break;
    }
    /* - Sequential Switch takes values from data fields starting from data1 and uses it as TX ID - */
    /* - ranged TX ID is now supported also - */
    case kGDXSequentialTX:
    {
        bool range = false; int cnt = 3; int tx = 0;
        // set range of TX ID if data2 and data3 is empty.
        if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

            // data1 must be less than data4
            if (pXSprite->data1 > pXSprite->data4) {
                int tmp = pXSprite->data1;
                pXSprite->data1 = (short)pXSprite->data4;
                pXSprite->data4 = tmp;
            }
            
            // force send command to all TX id in a range
            if (pSprite->hitag == 1) {
                for (int i = pXSprite->data1; i <= pXSprite->data4; i++) {
                    evSend(nSprite, 3, i, (COMMAND_ID) pXSprite->command);
                }

                pXSprite->txIndex = 0;
                SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                break;
            }

            // Make sure txIndex is correct as we store current index of TX ID here.
            if (pXSprite->txIndex < pXSprite->data1) pXSprite->txIndex = pXSprite->data1;
            else if (pXSprite->txIndex > pXSprite->data4) pXSprite->txIndex = pXSprite->data4;

            range = true;

        } else {
            // Make sure txIndex is correct as we store current index of data field here.
            if (pXSprite->txIndex > 3) pXSprite->txIndex = 0;
            else if (pXSprite->txIndex < 0) pXSprite->txIndex = 3;
        }

        switch (a3.cmd) {
            case COMMAND_ID_0:
                if (range == false) {
                    while (cnt-- >= 0) { // skip empty data fields
                        pXSprite->txIndex--;
                        if (pXSprite->txIndex < 0) pXSprite->txIndex = 3;
                        tx = GetDataVal(pSprite, pXSprite->txIndex);
                        if (tx < 0) ThrowError(" -- Current data index is negative");
                        if (tx > 0) break;
                        continue;
                    }
                } else {
                    pXSprite->txIndex--;
                    if (pXSprite->txIndex < pXSprite->data1) {
                        pXSprite->txIndex = pXSprite->data4;
                    }
                    tx = pXSprite->txIndex;
                }
                break;

            default:
                if (range == false) {
                    while (cnt-- >= 0) { // skip empty data fields
                        if (pXSprite->txIndex > 3) pXSprite->txIndex = 0;
                        tx = GetDataVal(pSprite, pXSprite->txIndex);
                        if (tx < 0) ThrowError(" ++ Current data index is negative");
                        pXSprite->txIndex++;
                        if (tx > 0) break;
                        continue;
                    }
                } else {
                    tx = pXSprite->txIndex;
                    if (pXSprite->txIndex >= pXSprite->data4) {
                        pXSprite->txIndex = pXSprite->data1;
                        break;
                    }
                    pXSprite->txIndex++;
                }
                break;
        }

        pXSprite->txID = (short)tx;
        SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
        break;
    }
    case 413:
        if (pXSprite->health > 0)
        {
            if (a3.cmd == 1)
            {
                if (SetSpriteState(nSprite, pXSprite, 1))
                {
                    seqSpawn(38, 3, pSprite->extra, nMGunOpenClient);
                    if (pXSprite->data1 > 0)
                        pXSprite->data2 = pXSprite->data1;
                }
            }
            else if (a3.cmd == 0)
            {
                if (SetSpriteState(nSprite, pXSprite, 0))
                    seqSpawn(40, 3, pSprite->extra, -1);
            }
        }
        break;
    case 414:
        if (SetSpriteState(nSprite, pXSprite, 1))
            pSprite->hitag |= 7;
        break;
    case 408:
        if (SetSpriteState(nSprite, pXSprite, 0))
            actPostSprite(nSprite, kStatFree);
        break;
    case 405:
        if (SetSpriteState(nSprite, pXSprite, 0))
            actPostSprite(nSprite, kStatFree);
        break;
    case 456:
        switch (a3.cmd)
        {
        case 0:
            pXSprite->state = 0;
            pSprite->cstat |= 32768;
            pSprite->cstat &= ~1;
            break;
        case 1:
            pXSprite->state = 1;
            pSprite->cstat &= (unsigned short)~32768;
            pSprite->cstat |= 1;
            break;
        case 3:
            pXSprite->state ^= 1;
            pSprite->cstat ^= 32768;
            pSprite->cstat ^= 1;
            break;
        }
        break;
    case 452:
        if (a3.cmd == 1)
        {
            if (SetSpriteState(nSprite, pXSprite, 1))
            {
                seqSpawn(38, 3, pSprite->extra, -1);
                sfxPlay3DSound(pSprite, 441, 0, 0);
            }
        }
        else if (a3.cmd == 0)
        {
            if (SetSpriteState(nSprite, pXSprite, 0))
            {
                seqSpawn(40, 3, pSprite->extra, -1);
                sfxKill3DSound(pSprite, 0, -1);
            }
        }
        break;
    case 23:
        switch (a3.cmd)
        {
        case 0:
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case 1:
            if (SetSpriteState(nSprite, pXSprite, 1))
                seqSpawn(37, 3, pSprite->extra, -1);
            break;
        default:
            SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            if (pXSprite->state)
                seqSpawn(37, 3, pSprite->extra, -1);
            break;
        }
        break;
    case 20:
        switch (a3.cmd)
        {
        case 0:
            if (SetSpriteState(nSprite, pXSprite, 0))
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            break;
        case 1:
            if (SetSpriteState(nSprite, pXSprite, 1))
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
            break;
        default:
            if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1))
            {
                if (pXSprite->state)
                    sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else
                    sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            }
            break;
        }
        break;
    case 21:
        switch (a3.cmd)
        {
        case 0:
            if (SetSpriteState(nSprite, pXSprite, 0))
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            break;
        case 1:
            if (SetSpriteState(nSprite, pXSprite, 1))
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
            break;
        default:
            if (SetSpriteState(nSprite, pXSprite, pXSprite->restState ^ 1))
            {
                if (pXSprite->state)
                    sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else
                    sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            }
            break;
        }
        break;
    // By NoOne: add linking for path markers and stacks feature
    case kMarkerLowWater:
    case kMarkerUpWater:
    case kMarkerUpGoo:
    case kMarkerLowGoo:
    case kMarkerUpLink:
    case kMarkerLowLink:
    case kMarkerUpStack:
    case kMarkerLowStack:
    case kMarkerPath:
        if (pXSprite->command == 5 && pXSprite->txID != 0)
            evSend(nSprite, 3, pXSprite->txID, COMMAND_ID_5);
        break;
    case 22:
        switch (a3.cmd)
        {
        case 0:
            pXSprite->data1--;
            if (pXSprite->data1 < 0)
                pXSprite->data1 += pXSprite->data3;
            break;
        default:
            pXSprite->data1++;
            if (pXSprite->data1 >= pXSprite->data3)
                pXSprite->data1 -= pXSprite->data3;
            break;
        }
        if (pXSprite->command == 5 && pXSprite->txID)
            evSend(nSprite, 3, pXSprite->txID, COMMAND_ID_5);
        sfxPlay3DSound(pSprite, pXSprite->data4, -1, 0);
        if (pXSprite->data1 == pXSprite->data2)
            SetSpriteState(nSprite, pXSprite, 1);
        else
            SetSpriteState(nSprite, pXSprite, 0);
        break;
    case kGDXObjPropertiesChanger:
    case kGDXObjPicnumChanger:
    case kGDXObjSizeChanger:
    case kGDXSectorFXChanger:
    case kGDXObjDataChanger:
        // by NoOne: Sending new command instead of link is *required*, because types above 
        //are universal and can paste properties in different objects.
        if (pXSprite->command == COMMAND_ID_5)
            evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
        else {
            evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste); // send first command to change properties
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID) pXSprite->command); // then send normal command
        }
        break;
    // this type damages sprite with given damageType
    case kGDXSpriteDamager:
        evSend(nSprite, 3, pXSprite->txID, kGDXCommandSpriteDamage);
        break;
    case 40: // Random weapon
    case 80: // Random ammo
        // let's first search for previously dropped items and remove it
        if (pXSprite->dropMsg > 0) {
            for (short nItem = headspritestat[3]; nItem >= 0; nItem = nextspritestat[nItem]) {
                spritetype* pItem = &sprite[nItem];
                if (pItem->lotag == pXSprite->dropMsg && pItem->x == pSprite->x && pItem->y == pSprite->y && pItem->z == pSprite->z) {
                    gFX.fxSpawn((FX_ID) 29, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
                    deletesprite(nItem);
                    break;
                }
            }
        }
        // then drop item
        DropRandomPickupObject(pSprite, pXSprite->dropMsg);
        break;
    case kGDXCustomDudeSpawn:
        if (gGameOptions.nMonsterSettings && actSpawnCustomDude(pSprite, -1) != NULL)
            gKillMgr.sub_263E0(1);
        break;
    case 18:
        if (gGameOptions.nMonsterSettings && pXSprite->data1 >= kDudeBase && pXSprite->data1 < kDudeMax)
        {
            
            spritetype* pSpawn = NULL;
            // By NoOne: add spawn random dude feature - works only if at least 2 data fields are not empty.
            if (!VanillaMode()) {
                if ((pSpawn = spawnRandomDude(pSprite)) == NULL)
                    pSpawn = actSpawnDude(pSprite, pXSprite->data1, -1, 0);
            } else {
              pSpawn = actSpawnDude(pSprite, pXSprite->data1, -1, 0);
            }

            if (pSpawn)
            {
                XSPRITE *pXSpawn = &xsprite[pSpawn->extra];
                gKillMgr.sub_263E0(1);
                switch (pXSprite->data1)
                {
                case 239:
                case 240:
                case 242:
                case 252:
                case 253:
                {
                    pXSpawn->health = dudeInfo[pXSprite->data1 - kDudeBase].startHealth << 4;
                    pXSpawn->burnTime = 10;
                    pXSpawn->target = -1;
                    aiActivateDude(pSpawn, pXSpawn);
                    break;
                }
                }
            }
        }
        break;
    case 19:
        pXSprite->triggerOn = 0;
        pXSprite->isTriggered = 1;
        SetSpriteState(nSprite, pXSprite, 1);
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            spritetype *pPlayerSprite = gPlayer[p].pSprite;
            int dx = (pSprite->x - pPlayerSprite->x)>>4;
            int dy = (pSprite->y - pPlayerSprite->y)>>4;
            int dz = (pSprite->z - pPlayerSprite->z)>>8;
            int nDist = dx*dx+dy*dy+dz*dz+0x40000;
            gPlayer[p].at37f = divscale16(pXSprite->data1, nDist);
        }
        break;
    case 400:
        if (pSprite->hitag&16)
            return;
        fallthrough__;
    case 418:
    case 419:
    case 420:
        actExplodeSprite(pSprite);
        break;
    case 459:
        switch (a3.cmd)
        {
        case 1:
            SetSpriteState(nSprite, pXSprite, 1);
            break;
        default:
            pSprite->cstat &= (unsigned short)~32768;
            actExplodeSprite(pSprite);
            break;
        }
        break;
    case kGDXSeqSpawner:
    case kGDXEffectSpawner:
        switch (a3.cmd) {
            case COMMAND_ID_0:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case COMMAND_ID_1:
                if (pXSprite->state == 1) break;
                fallthrough__;
            case COMMAND_ID_21:
                SetSpriteState(nSprite, pXSprite, 1);
                if (pXSprite->txID <= 0)
                    (pSprite->type == kGDXSeqSpawner) ? useSeqSpawnerGen(pXSprite, NULL) : useEffectGen(pXSprite, NULL);
                else if (pXSprite->command == COMMAND_ID_5)
                    evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                else {
                    evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID) pXSprite->command);
                }
            
                if (pXSprite->busyTime > 0)
                    evPost(nSprite, 3, ClipLow((int(pXSprite->busyTime) + Random2(pXSprite->data1)) * 120 / 10, 0), COMMAND_ID_21);
                break;

            case COMMAND_ID_3:
                if (pXSprite->state == 0) evPost(nSprite, 3, 0, COMMAND_ID_21);
                else evPost(nSprite, 3, 0, COMMAND_ID_0);
                break;
        }

        break;
    case 402:
        if (pSprite->statnum == 8)
            break;
        if (a3.cmd != 1)
            actExplodeSprite(pSprite);
        else
        {
            sfxPlay3DSound(pSprite, 454, 0, 0);
            evPost(nSprite, 3, 18, COMMAND_ID_0);
        }
        break;
    case 401:
    case kGDXThingTNTProx:
        if (pSprite->statnum == 8)
            break;
        switch (a3.cmd)
        {
        case 35:
            if (!pXSprite->state)
            {
                sfxPlay3DSound(pSprite, 452, 0, 0);
                evPost(nSprite, 3, 30, COMMAND_ID_0);
                pXSprite->state = 1;
            }
            break;
        case 1:
            sfxPlay3DSound(pSprite, 451, 0, 0);
            pXSprite->Proximity = 1;
            break;
        default:
            actExplodeSprite(pSprite);
            break;
        }
        break;
    case 431:
        sub_43CF8(pSprite, pXSprite, a3);
        break;
    case kGDXThingCustomDudeLifeLeech:
        dudeLeechOperate(pSprite, pXSprite, a3);
        break;
    case kGDXWindGenerator:
        switch (a3.cmd) {
        case COMMAND_ID_0:
            stopWindOnSectors(pXSprite);
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case COMMAND_ID_1:
            if (pXSprite->state == 1) break;
            fallthrough__;
        case COMMAND_ID_21:
            SetSpriteState(nSprite, pXSprite, 1);
            if (pXSprite->txID <= 0) useSectorWindGen(pXSprite, NULL);
            else if (pXSprite->command == COMMAND_ID_5)
                evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
            else {
                evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID) pXSprite->command);
            }

            if (pXSprite->busyTime > 0)
                evPost(nSprite, 3, pXSprite->busyTime, COMMAND_ID_21);
            break;
        case COMMAND_ID_3:
            if (pXSprite->state == 0) evPost(nSprite, 3, 0, COMMAND_ID_21);
            else evPost(nSprite, 3, 0, COMMAND_ID_0);
            break;
        }

        break;
    case kGDXDudeTargetChanger:
    {
        // this one is required if data4 of generator was dynamically changed
        // it turns monsters in normal idle state instead of genIdle, so they
        // not ignore the world.
        bool activated = false;
        if (pXSprite->dropMsg == 3 && 3 != pXSprite->data4) {
            activateDudes(pXSprite->txID);
            activated = true;
        }

        switch (a3.cmd)
        {
        case COMMAND_ID_0:
            if (pXSprite->data4 == 3 && activated == false) activateDudes(pXSprite->txID);
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case COMMAND_ID_1:
            if (pXSprite->state == 1) break;
            fallthrough__;
        case COMMAND_ID_21:
            SetSpriteState(nSprite, pXSprite, 1);
            if (pXSprite->txID <= 0 || !getDudesForTargetChg(pXSprite)) {
                evPost(nSprite, 3, 0, COMMAND_ID_0);
                break;
            }
            else if (pXSprite->command == COMMAND_ID_5)
                evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
            else {
                evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
            }

            if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, COMMAND_ID_21);
            break;
        case COMMAND_ID_3:
            if (pXSprite->state == 0) evPost(nSprite, 3, 0, COMMAND_ID_21);
            else evPost(nSprite, 3, 0, COMMAND_ID_0);
            break;
        }

        pXSprite->dropMsg = (short)pXSprite->data4;
        break;
    }
    case kGDXObjDataAccumulator:
            switch (a3.cmd) {
            case COMMAND_ID_0:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case COMMAND_ID_1:
                if (pXSprite->state == 1) break;
            case COMMAND_ID_21:
                SetSpriteState(nSprite, pXSprite, 1);

                // force OFF after *all* TX objects reach the goal value
                if (pSprite->hitag == 0 && goalValueIsReached(pXSprite)) {
                    evPost(nSprite, 3, 0, COMMAND_ID_0);
                    break;
                }

                if (pXSprite->data1 > 0 && pXSprite->data1 <= 4 && pXSprite->data2 > 0) {
                    if (pXSprite->txID != 0) {
                        if (pXSprite->command == COMMAND_ID_5)
                            evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                        else {
                            evSend(nSprite, 3, pXSprite->txID, kGDXCommandPaste);
                            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID) pXSprite->command);
                        }
                    }
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, COMMAND_ID_21);
                }
                break;
            case COMMAND_ID_3:
                if (pXSprite->state == 0) evPost(nSprite, 3, 0, COMMAND_ID_21);
                else evPost(nSprite, 3, 0, COMMAND_ID_0);
                break;
            }
        break;
    case 700:
    case 701:
    case 702:
    case 703:
    case 704:
    case 705:
    case 706:
    case 707:
    case 708:
        switch (a3.cmd)
        {
        case 0:
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case 21:
            if (pSprite->type != 700)
                ActivateGenerator(nSprite);
            if (pXSprite->txID)
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
            if (pXSprite->busyTime > 0)
            {
                int nRand = Random2(pXSprite->data1);
                evPost(nSprite, 3, 120*(nRand+pXSprite->busyTime) / 10, COMMAND_ID_21);
            }
            break;
        default:
            if (!pXSprite->state)
            {
                SetSpriteState(nSprite, pXSprite, 1);
                evPost(nSprite, 3, 0, COMMAND_ID_21);
            }
            break;
        }
        break;
    case 711:
        if (gGameOptions.nGameType == 0)
        {
            if (gMe->pXSprite->health <= 0)
                break;
            gMe->at30a = 0;
        }
        sndStartSample(pXSprite->data1, -1, 1, 0);
        break;
    case 416:
    case 417:
    case 425:
    case 426:
    case 427:
        switch (a3.cmd)
        {
        case 0:
            if (SetSpriteState(nSprite, pXSprite, 0))
                actActivateGibObject(pSprite, pXSprite);
            break;
        case 1:
            if (SetSpriteState(nSprite, pXSprite, 1))
                actActivateGibObject(pSprite, pXSprite);
            break;
        default:
            if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1))
                actActivateGibObject(pSprite, pXSprite);
            break;
        }
        break;
    default:
        switch (a3.cmd)
        {
        case 0:
            SetSpriteState(nSprite, pXSprite, 0);
            break;
        case 1:
            SetSpriteState(nSprite, pXSprite, 1);
            break;
        default:
            SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            break;
        }
        break;
    }
}

// by NoOne: this function stops wind on all TX sectors affected by WindGen after it goes off state.
void stopWindOnSectors(XSPRITE* pXSource) {
    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type != 3) continue;
        XSECTOR * pXSector = &xsector[sector[rxBucket[i].index].extra];
        if ((pXSector->state == 1 && !pXSector->windAlways) || sprite[pXSource->reference].hitag == 1)
            pXSector->windVel = 0;
    }
}

void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = NULL; bool forceWind = false;

    if (pSector == NULL) {
        
        if (sector[pSource->sectnum].extra < 0) {
            int nXSector = dbInsertXSector(pSource->sectnum);
            if (nXSector > 0) pXSector = &xsector[nXSector];
            else return;

            forceWind = true;

        } else {
            pXSector = &xsector[sector[pSource->sectnum].extra];
        }
    } else {
        pXSector = &xsector[pSector->extra];
    }
    
    if (pSource->hitag) {
        pXSector->panAlways = 1;
        pXSector->windAlways = 1;
    } else if (forceWind) 
        pXSector->windAlways = 1;

    if (pXSource->data2 > 32766) pXSource->data2 = 32767;

    if (pXSource->data1 == 1 || pXSource->data1 == 3) pXSector->windVel = Random(pXSource->data2);
    else pXSector->windVel = pXSource->data2;

    if (pXSource->data1 == 2 || pXSource->data1 == 3) {
        short ang = pSource->ang;
        while (pSource->ang == ang)
            pSource->ang = Random3(kAng360);
    }

    pXSector->windAng = pSource->ang;

    if (pXSource->data3 > 0 && pXSource->data3 < 4) {
        switch (pXSource->data3) {
        case 1:
            pXSector->panFloor = true;
            pXSector->panCeiling = false;
            break;
        case 2:
            pXSector->panFloor = false;
            pXSector->panCeiling = true;
            break;
        case 3:
            pXSector->panFloor = true;
            pXSector->panCeiling = true;
            break;
        }

        pXSector->panAngle = pXSector->windAng;
        pXSector->panVel = pXSector->windVel;
    }
}

void useSeqSpawnerGen(XSPRITE* pXSource, spritetype* pSprite) {
    if (pSprite == NULL) pSprite = &sprite[pXSource->reference];
    if (pSprite->extra < 0) return;
    
    seqSpawn(pXSource->data2, 3, pSprite->extra, (pXSource->data3 > 0) ? pXSource->data3 : -1);
    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSprite, pXSource->data4, -1, 0);
}

void useEffectGen(XSPRITE* pXSource, spritetype* pSprite) {
    if (pSprite == NULL) pSprite = &sprite[pXSource->reference];
    if (pSprite->extra < 0) return;

    int top, bottom; GetSpriteExtents(pSprite, &top, &bottom); int cnt = pXSource->data4;
    spritetype* pEffect = NULL; if (cnt > 32) cnt = 32;

    while (cnt-- >= 0) {
        if (cnt > 0) {

            int dx = Random3(250);
            int dy = Random3(150);

            pEffect = gFX.fxSpawn((FX_ID)pXSource->data2, pSprite->sectnum, pSprite->x + dx, pSprite->y + dy, top, 0);

        }
        else {
            pEffect = gFX.fxSpawn((FX_ID)pXSource->data2, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
        }

        if (pEffect != NULL) {
            if (pEffect->pal <= 0) pEffect->pal = pSprite->pal;
            if (pEffect->xrepeat <= 0) pEffect->xrepeat = pSprite->xrepeat;
            if (pEffect->yrepeat <= 0) pEffect->yrepeat = pSprite->yrepeat;
            if (pEffect->shade == 0) pEffect->shade = pSprite->shade;
        }
    }

    if (pXSource->data3 > 0)
        sfxPlay3DSound(pSprite, pXSource->data3, -1, 0);

}



void SetupGibWallState(walltype *pWall, XWALL *pXWall)
{
    walltype *pWall2 = NULL;
    if (pWall->nextwall >= 0)
        pWall2 = &wall[pWall->nextwall];
    if (pXWall->state)
    {
        pWall->cstat &= ~65;
        if (pWall2)
        {
            pWall2->cstat &= ~65;
            pWall->cstat &= ~16;
            pWall2->cstat &= ~16;
        }
        return;
    }
    char bVector = pXWall->triggerVector != 0;
    pWall->cstat |= 1;
    if (bVector)
        pWall->cstat |= 64;
    if (pWall2)
    {
        pWall2->cstat |= 1;
        if (bVector)
            pWall2->cstat |= 64;
        pWall->cstat |= 16;
        pWall2->cstat |= 16;
    }
}

void OperateWall(int nWall, XWALL *pXWall, EVENT a3)
{
    walltype *pWall = &wall[nWall];
    switch (a3.cmd)
    {
    case 6:
        pXWall->locked = 1;
        return;
    case 7:
        pXWall->locked = 0;
        return;
    case 8:
        pXWall->locked ^= 1;
        return;
    }
    if (pWall->lotag == 511)
    {
        char bStatus;
        switch (a3.cmd)
        {
        case 1:
        case 51:
            bStatus = SetWallState(nWall, pXWall, 1);
            break;
        case 0:
            bStatus = SetWallState(nWall, pXWall, 0);
            break;
        default:
            bStatus = SetWallState(nWall, pXWall, pXWall->state^1);
            break;
        }
        if (bStatus)
        {
            SetupGibWallState(pWall, pXWall);
            if (pXWall->state)
            {
                CGibVelocity vel(100, 100, 250);
                int nType = ClipRange(pXWall->data, 0, 31);
                if (nType > 0)
                    GibWall(nWall, (GIBTYPE)nType, &vel);
            }
        }
        return;
    }
    switch (a3.cmd)
    {
    case 0:
        SetWallState(nWall, pXWall, 0);
        break;
    case 1:
        SetWallState(nWall, pXWall, 1);
        break;
    default:
        SetWallState(nWall, pXWall, pXWall->state ^ 1);
        break;
    }
}

void SectorStartSound(int nSector, int nState)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 0 && pSprite->type == 709)
        {
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (nState)
            {
                if (pXSprite->data3)
                    sfxPlay3DSound(pSprite, pXSprite->data3, 0, 0);
            }
            else
            {
                if (pXSprite->data1)
                    sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
            }
        }
    }
}

void SectorEndSound(int nSector, int nState)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 0 && pSprite->type == 709)
        {
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (nState)
            {
                if (pXSprite->data2)
                    sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            }
            else
            {
                if (pXSprite->data4)
                    sfxPlay3DSound(pSprite, pXSprite->data4, 0, 0);
            }
        }
    }
}

void PathSound(int nSector, int nSound)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 0 && pSprite->type == 709)
            sfxPlay3DSound(pSprite, nSound, 0, 0);
    }
}

void DragPoint(int nWall, int x, int y)
{
    viewInterpolateWall(nWall, &wall[nWall]);
    wall[nWall].x = x;
    wall[nWall].y = y;

    int vsi = numwalls;
    int vb = nWall;
    do
    {
        if (wall[vb].nextwall >= 0)
        {
            vb = wall[wall[vb].nextwall].point2;
            viewInterpolateWall(vb, &wall[vb]);
            wall[vb].x = x;
            wall[vb].y = y;
        }
        else
        {
            vb = nWall;
            do
            {
                if (wall[lastwall(vb)].nextwall >= 0)
                {
                    vb = wall[lastwall(vb)].nextwall;
                    viewInterpolateWall(vb, &wall[vb]);
                    wall[vb].x = x;
                    wall[vb].y = y;
                }
                else
                    break;
                vsi--;
            } while (vb != nWall && vsi > 0);
            break;
        }
        vsi--;
    } while (vb != nWall && vsi > 0);
}

void TranslateSector(int nSector, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12)
{
    int x, y;
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = &xsector[nXSector];
    int v20 = interpolate(a6, a9, a2);
    int vc = interpolate(a6, a9, a3);
    int v28 = vc - v20;
    int v24 = interpolate(a7, a10, a2);
    int v8 = interpolate(a7, a10, a3);
    int v2c = v8 - v24;
    int v44 = interpolate(a8, a11, a2);
    int vbp = interpolate(a8, a11, a3);
    int v14 = vbp - v44;
    int nWall = sector[nSector].wallptr;
    if (a12)
    {
        for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
        {
            x = baseWall[nWall].x;
            y = baseWall[nWall].y;
            if (vbp)
                RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
            DragPoint(nWall, x+vc-a4, y+v8-a5);
        }
    }
    else
    {
        for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
        {
            int v10 = wall[nWall].point2;
            x = baseWall[nWall].x;
            y = baseWall[nWall].y;
            if (wall[nWall].cstat&16384)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
                DragPoint(nWall, x+vc-a4, y+v8-a5);
                if ((wall[v10].cstat&49152) == 0)
                {
                    x = baseWall[v10].x;
                    y = baseWall[v10].y;
                    if (vbp)
                        RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
                    DragPoint(v10, x+vc-a4, y+v8-a5);
                }
                continue;
            }
            if (wall[nWall].cstat&32768)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, -vbp, a4, a5);
                DragPoint(nWall, x-(vc-a4), y-(v8-a5));
                if ((wall[v10].cstat&49152) == 0)
                {
                    x = baseWall[v10].x;
                    y = baseWall[v10].y;
                    if (vbp)
                        RotatePoint((int*)&x, (int*)&y, -vbp, a4, a5);
                    DragPoint(v10, x-(vc-a4), y-(v8-a5));
                }
                continue;
            }
        }
    }
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        // By NoOne: allow to move markers by sector movements in game if hitag 1 is added in editor.
        if (pSprite->statnum == 10 || pSprite->statnum == 16) {
            if (!(pSprite->hitag&kHitagExtBit)) continue;
        }
        x = baseSprite[nSprite].x;
        y = baseSprite[nSprite].y;
        if (sprite[nSprite].cstat&8192)
        {
            if (vbp)
                RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
            viewBackupSpriteLoc(nSprite, pSprite);
            pSprite->ang = (pSprite->ang+v14)&2047;
            pSprite->x = x+vc-a4;
            pSprite->y = y+v8-a5;
        }
        else if (sprite[nSprite].cstat&16384)
        {
            if (vbp)
                RotatePoint((int*)& x, (int*)& y, -vbp, a4, a4);
            viewBackupSpriteLoc(nSprite, pSprite);
            pSprite->ang = (pSprite->ang-v14)&2047;
            pSprite->x = x-(vc-a4);
            pSprite->y = y-(v8-a5);
        }
        else if (pXSector->Drag)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            int floorZ = getflorzofslope(nSector, pSprite->x, pSprite->y);
            if (!(pSprite->cstat&48) && floorZ <= bottom)
            {
                if (v14)
                    RotatePoint((int*)&pSprite->x, (int*)&pSprite->y, v14, v20, v24);
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->ang = (pSprite->ang+v14)&2047;
                pSprite->x += v28;
                pSprite->y += v2c;
            }
        }
    }
}

void ZTranslateSector(int nSector, XSECTOR *pXSector, int a3, int a4)
{
    sectortype *pSector = &sector[nSector];
    viewInterpolateSector(nSector, pSector);
    int dz = pXSector->at28_0-pXSector->at24_0;
    if (dz != 0)
    {
        int oldZ = pSector->floorz;
        baseFloor[nSector] = pSector->floorz = pXSector->at24_0 + mulscale16(dz, GetWaveValue(a3, a4));
        velFloor[nSector] += (pSector->floorz-oldZ)<<8;
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->statnum == 10 || pSprite->statnum == 16)
                continue;
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (pSprite->cstat&8192)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->floorz-oldZ;
            }
            else if (pSprite->hitag&2)
                pSprite->hitag |= 4;
            else if (oldZ <= bottom && !(pSprite->cstat&48))
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->floorz-oldZ;
            }
        }
    }
    dz = pXSector->at20_0-pXSector->at1c_0;
    if (dz != 0)
    {
        int oldZ = pSector->ceilingz;
        baseCeil[nSector] = pSector->ceilingz = pXSector->at1c_0 + mulscale16(dz, GetWaveValue(a3, a4));
        velCeil[nSector] += (pSector->ceilingz-oldZ)<<8;
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->statnum == 10 || pSprite->statnum == 16)
                continue;
            if (pSprite->cstat&16384)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->ceilingz-oldZ;
            }
        }
    }
}

int GetHighestSprite(int nSector, int nStatus, int *a3)
{
    *a3 = sector[nSector].floorz;
    int v8 = -1;
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].statnum == nStatus || nStatus == 1024)
        {
            spritetype *pSprite = &sprite[nSprite];
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (top-pSprite->z > *a3)
            {
                *a3 = top-pSprite->z;
                v8 = nSprite;
            }
        }
    }
    return v8;
}

int GetCrushedSpriteExtents(unsigned int nSector, int *pzTop, int *pzBot)
{
    dassert(pzTop != NULL && pzBot != NULL);
    dassert(nSector < (unsigned int)numsectors);
    int vc = -1;
    sectortype *pSector = &sector[nSector];
    int vbp = pSector->ceilingz;
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 6 || pSprite->statnum == 4)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (vbp > top)
            {
                vbp = top;
                *pzTop = top;
                *pzBot = bottom;
                vc = nSprite;
            }
        }
    }
    return vc;
}

int VCrushBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->at7_2;
    else
        nWave = pXSector->at7_5;
    int dz1 = pXSector->at20_0 - pXSector->at1c_0;
    int vc = pXSector->at1c_0;
    if (dz1 != 0)
        vc += mulscale16(dz1, GetWaveValue(a2, nWave));
    int dz2 = pXSector->at28_0 - pXSector->at24_0;
    int v10 = pXSector->at24_0;
    if (dz2 != 0)
        v10 += mulscale16(dz2, GetWaveValue(a2, nWave));
    int v18;
    if (GetHighestSprite(nSector, 6, &v18) >= 0 && vc >= v18)
        return 1;
    viewInterpolateSector(nSector, &sector[nSector]);
    if (dz1 != 0)
        sector[nSector].ceilingz = vc;
    if (dz2 != 0)
        sector[nSector].floorz = v10;
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int VSpriteBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->at7_2;
    else
        nWave = pXSector->at7_5;
    int dz1 = pXSector->at28_0 - pXSector->at24_0;
    if (dz1 != 0)
    {
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->cstat&8192)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z = baseSprite[nSprite].z+mulscale16(dz1, GetWaveValue(a2, nWave));
            }
        }
    }
    int dz2 = pXSector->at20_0 - pXSector->at1c_0;
    if (dz2 != 0)
    {
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->cstat&16384)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z = baseSprite[nSprite].z+mulscale16(dz2, GetWaveValue(a2, nWave));
            }
        }
    }
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int VDoorBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int vbp;
    if (pXSector->state)
        vbp = 65536/ClipLow((120*pXSector->busyTimeA)/10, 1);
    else
        vbp = -65536/ClipLow((120*pXSector->busyTimeB)/10, 1);
    int top, bottom;
    int nSprite = GetCrushedSpriteExtents(nSector,&top,&bottom);
    if (nSprite >= 0 && a2 > pXSector->busy)
    {
        spritetype *pSprite = &sprite[nSprite];
        dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSector->at20_0 > pXSector->at1c_0 || pXSector->at28_0 < pXSector->at24_0)
        {
            if (pXSector->interruptable)
            {
                if (pXSector->Crush)
                {
                    if (pXSprite->health <= 0)
                        return 2;
                    int nDamage;
                    if (pXSector->data == 0)
                        nDamage = 500;
                    else
                        nDamage = pXSector->data;
                    actDamageSprite(nSprite, &sprite[nSprite], DAMAGE_TYPE_0, nDamage<<4);
                }
                a2 = ClipRange(a2-(vbp/2)*4, 0, 65536);
            }
            else if (pXSector->Crush && pXSprite->health > 0)
            {
                int nDamage;
                if (pXSector->data == 0)
                    nDamage = 500;
                else
                    nDamage = pXSector->data;
                actDamageSprite(nSprite, &sprite[nSprite], DAMAGE_TYPE_0, nDamage<<4);
                a2 = ClipRange(a2-(vbp/2)*4, 0, 65536);
            }
        }
    }
    else if (nSprite >= 0 && a2 < pXSector->busy)
    {
        spritetype *pSprite = &sprite[nSprite];
        dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSector->at1c_0 > pXSector->at20_0 || pXSector->at24_0 < pXSector->at28_0)
        {
            if (pXSector->interruptable)
            {
                if (pXSector->Crush)
                {
                    if (pXSprite->health <= 0)
                        return 2;
                    int nDamage;
                    if (pXSector->data == 0)
                        nDamage = 500;
                    else
                        nDamage = pXSector->data;
                    actDamageSprite(nSprite, &sprite[nSprite], DAMAGE_TYPE_0, nDamage<<4);
                }
                a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
            }
            else if (pXSector->Crush && pXSprite->health > 0)
            {
                int nDamage;
                if (pXSector->data == 0)
                    nDamage = 500;
                else
                    nDamage = pXSector->data;
                actDamageSprite(nSprite, &sprite[nSprite], DAMAGE_TYPE_0, nDamage<<4);
                a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
            }
        }
    }
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->at7_2;
    else
        nWave = pXSector->at7_5;
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int HDoorBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->at7_2;
    else
        nWave = pXSector->at7_5;
    spritetype *pSprite1 = &sprite[pXSector->at2c_0];
    spritetype *pSprite2 = &sprite[pXSector->at2e_0];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->lotag == 616);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int RDoorBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->at7_2;
    else
        nWave = pXSector->at7_5;
    spritetype *pSprite = &sprite[pXSector->at2c_0];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, 0, pSprite->x, pSprite->y, pSprite->ang, pSector->lotag == 617);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int StepRotateBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    spritetype *pSprite = &sprite[pXSector->at2c_0];
    int vbp;
    if (pXSector->busy < a2)
    {
        vbp = pXSector->data+pSprite->ang;
        int nWave = pXSector->at7_2;
        TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, pXSector->data, pSprite->x, pSprite->y, vbp, 1);
    }
    else
    {
        vbp = pXSector->data-pSprite->ang;
        int nWave = pXSector->at7_5;
        TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, vbp, pSprite->x, pSprite->y, pXSector->data, 1);
    }
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        pXSector->data = vbp&2047;
        return 3;
    }
    return 0;
}

int GenSectorBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    pXSector->busy = a2;
    if (pXSector->command == 5 && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, COMMAND_ID_5);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int PathBusy(unsigned int nSector, unsigned int a2)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    spritetype *pSprite = &sprite[basePath[nSector]];
    spritetype *pSprite1 = &sprite[pXSector->at2c_0];
    XSPRITE *pXSprite1 = &xsprite[pSprite1->extra];
    spritetype *pSprite2 = &sprite[pXSector->at2e_0];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nWave = pXSprite1->wave;
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, 1);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if ((a2&0xffff) == 0)
    {
        evPost(nSector, 6, (120*pXSprite2->waitTime)/10, COMMAND_ID_1);
        pXSector->state = 0;
        pXSector->busy = 0;
        if (pXSprite1->data4)
            PathSound(nSector, pXSprite1->data4);
        pXSector->at2c_0 = pXSector->at2e_0;
        pXSector->data = pXSprite2->data1;
        return 3;
    }
    return 0;
}

void OperateDoor(unsigned int nSector, XSECTOR *pXSector, EVENT a3, BUSYID a4) 
{
    switch (a3.cmd)
    {
    case 0:
        if (pXSector->busy)
        {
            AddBusy(nSector, a4, -65536/ClipLow((pXSector->busyTimeB*120)/10, 1));
            SectorStartSound(nSector, 1);
        }
        break;
    case 1:
        if (pXSector->busy != 0x10000)
        {
            AddBusy(nSector, a4, 65536/ClipLow((pXSector->busyTimeA*120)/10, 1));
            SectorStartSound(nSector, 0);
        }
        break;
    default:
        if (pXSector->busy&0xffff)
        {
            if (pXSector->interruptable)
            {
                ReverseBusy(nSector, a4);
                pXSector->state = !pXSector->state;
            }
        }
        else
        {
            char t = !pXSector->state;
            int nDelta;
            if (t)
                nDelta = 65536/ClipLow((pXSector->busyTimeA*120)/10, 1);
            else
                nDelta = -65536/ClipLow((pXSector->busyTimeB*120)/10, 1);
            AddBusy(nSector, a4, nDelta);
            SectorStartSound(nSector, pXSector->state);
        }
        break;
    }
}

char SectorContainsDudes(int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].statnum == 6)
            return 1;
    }
    return 0;
}

void TeleFrag(int nKiller, int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 6)
            actDamageSprite(nKiller, pSprite, DAMAGE_TYPE_3, 4000);
        else if (pSprite->statnum == 4)
            actDamageSprite(nKiller, pSprite, DAMAGE_TYPE_3, 4000);
    }
}

void OperateTeleport(unsigned int nSector, XSECTOR *pXSector)
{
    dassert(nSector < (unsigned int)numsectors);
    int nDest = pXSector->at2c_0;
    dassert(nDest < kMaxSprites);
    spritetype *pDest = &sprite[nDest];
    dassert(pDest->statnum == kStatMarker);
    dassert(pDest->type == kMarkerWarpDest);
    dassert(pDest->sectnum >= 0 && pDest->sectnum < kMaxSectors);
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == 6)
        {
            PLAYER *pPlayer;
            char bPlayer = IsPlayerSprite(pSprite);
            if (bPlayer)
                pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
            else
                pPlayer = NULL;
            if (bPlayer || !SectorContainsDudes(pDest->sectnum))
            {
                if (!(gGameOptions.uNetGameFlags&2))
                    TeleFrag(pXSector->data, pDest->sectnum);
                pSprite->x = pDest->x;
                pSprite->y = pDest->y;
                pSprite->z += sector[pDest->sectnum].floorz-sector[nSector].floorz;
                pSprite->ang = pDest->ang;
                ChangeSpriteSect(nSprite, pDest->sectnum);
                sfxPlay3DSound(pDest, 201, -1, 0);
                xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
                ClearBitString(gInterpolateSprite, nSprite);
                viewBackupSpriteLoc(nSprite, pSprite);
                if (pPlayer)
                {
                    playerResetInertia(pPlayer);
                    pPlayer->at6b = pPlayer->at73 = 0;
                }
            }
        }
    }
}

void OperatePath(unsigned int nSector, XSECTOR *pXSector, EVENT a3)
{
    int nSprite;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite;
    dassert(nSector < (unsigned int)numsectors);
    spritetype *pSprite2 = &sprite[pXSector->at2c_0];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nId = pXSprite2->data2;
    for (nSprite = headspritestat[16]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == 15)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }
    if (nSprite < 0)
        ThrowError("Unable to find path marker with id #%d", nId);
    pXSector->at2e_0 = nSprite;
    pXSector->at24_0 = pSprite2->z;
    pXSector->at28_0 = pSprite->z;
    switch (a3.cmd)
    {
    case 1:
        pXSector->state = 0;
        pXSector->busy = 0;
        AddBusy(nSector, BUSYID_7, 65536/ClipLow((120*pXSprite2->busyTime)/10,1));
        if (pXSprite2->data3)
            PathSound(nSector, pXSprite2->data3);
        break;
    }
}

void OperateSector(unsigned int nSector, XSECTOR *pXSector, EVENT a3)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    switch (a3.cmd)
    {
    case 6:
        pXSector->locked = 1;
        break;
    case 7:
        pXSector->locked = 0;
        // By NoOne: reset counter sector state and make it work again after unlock, so it can be used again.
        // See callback.cpp for more info.
        if (pSector->lotag == kSecCounter) {
            pXSector->state = 0;
            evPost(nSector, 6, 0, CALLBACK_ID_12);
        }
        break;
    case 8:
        pXSector->locked ^= 1;
        // same as above...
        if (pSector->lotag == kSecCounter && pXSector->locked != 1) {
            pXSector->state = 0;
            evPost(nSector, 6, 0, CALLBACK_ID_12);
        }
        break;
    case 9:
        pXSector->at1b_2 = 0;
        pXSector->at1b_3 = 1;
        break;
    case 10:
        pXSector->at1b_2 = 1;
        pXSector->at1b_3 = 0;
        break;
    case 11:
        pXSector->at1b_2 = 1;
        pXSector->at1b_3 = 1;
        break;
    default:
        switch (pSector->lotag)
        {
        case 602:
            OperateDoor(nSector, pXSector, a3, BUSYID_1);
            break;
        case 600:
            OperateDoor(nSector, pXSector, a3, BUSYID_2);
            break;
        case 614:
        case 616:
            OperateDoor(nSector, pXSector, a3, BUSYID_3);
            break;
        case 615:
        case 617:
            OperateDoor(nSector, pXSector, a3, BUSYID_4);
            break;
        case 613:
            switch (a3.cmd)
            {
            case 1:
                pXSector->state = 0;
                pXSector->busy = 0;
                AddBusy(nSector, BUSYID_5, 65536/ClipLow((120*pXSector->busyTimeA)/10, 1));
                SectorStartSound(nSector, 0);
                break;
            case 0:
                pXSector->state = 1;
                pXSector->busy = 65536;
                AddBusy(nSector, BUSYID_5, -65536/ClipLow((120*pXSector->busyTimeB)/10, 1));
                SectorStartSound(nSector, 1);
                break;
            }
            break;
        case 604:
            OperateTeleport(nSector, pXSector);
            break;
        case 612:
            OperatePath(nSector, pXSector, a3);
            break;
        default:
            if (pXSector->busyTimeA || pXSector->busyTimeB)
                OperateDoor(nSector, pXSector, a3, BUSYID_6);
            else
            {
                switch (a3.cmd)
                {
                case 0:
                    SetSectorState(nSector, pXSector, 0);
                    break;
                case 1:
                    SetSectorState(nSector, pXSector, 1);
                    break;
                default:
                    SetSectorState(nSector, pXSector, pXSector->state^1);
                    break;
                }
            }
            break;
        }
        break;
    }
}

void InitPath(unsigned int nSector, XSECTOR *pXSector)
{
    int nSprite;
    spritetype *pSprite;
    XSPRITE *pXSprite;
    dassert(nSector < (unsigned int)numsectors);
    int nId = pXSector->data;
    for (nSprite = headspritestat[16]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == 15)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }
    if (nSprite < 0)
        ThrowError("Unable to find path marker with id #%d", nId);
    pXSector->at2c_0 = nSprite;
    basePath[nSector] = nSprite;
    if (pXSector->state)
        evPost(nSector, 6, 0, COMMAND_ID_1);
}

void LinkSector(int nSector, XSECTOR *pXSector, EVENT a3)
{
    sectortype *pSector = &sector[nSector];
    int nBusy = GetSourceBusy(a3);
    switch (pSector->lotag)
    {
    case 602:
        VSpriteBusy(nSector, nBusy);
        break;
    case 600:
        VDoorBusy(nSector, nBusy);
        break;
    case 614:
    case 616:
        HDoorBusy(nSector, nBusy);
        break;
    case 615:
    case 617:
        RDoorBusy(nSector, nBusy);
        break;
     /* By NoOne: add link support for counter sectors so they can change necessary type and count of types*/
    case kSecCounter:
    {
        int nXIndex;
        nXIndex = sector[a3.index].extra;
        XSECTOR* pXSector2 = &xsector[nXIndex];
        pXSector->waitTimeA = pXSector2->waitTimeA;
        pXSector->data = pXSector2->data;
        break;
    }
    default:
        pXSector->busy = nBusy;
        if ((pXSector->busy&0xffff) == 0)
            SetSectorState(nSector, pXSector, nBusy>>16);
        break;
    }
}

void LinkSprite(int nSprite, XSPRITE *pXSprite, EVENT a3)
{
    spritetype *pSprite = &sprite[nSprite];
    int nBusy = GetSourceBusy(a3);
    switch (pSprite->type)
    {
    
    //By NoOne: these can be linked too now, so it's possible to change palette, underwater status and more...
    case kMarkerLowWater:
    case kMarkerUpWater:
    case kMarkerUpGoo:
    case kMarkerLowGoo:
    case kMarkerUpLink:
    case kMarkerLowLink:
    case kMarkerUpStack:
    case kMarkerLowStack:
    {
        if (a3.type != 3) break;
        spritetype *pSprite2 = &sprite[a3.index];
        if (pSprite2->extra < 0) break;
        XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];

        // Only lower to lower and upper to upper linking allowed.
        switch (pSprite->type) {
        case kMarkerLowWater:
        case kMarkerLowLink:
        case kMarkerLowStack:
        case kMarkerLowGoo:
            switch (pSprite2->type) {
            case kMarkerLowWater:
            case kMarkerLowLink:
            case kMarkerLowStack:
            case kMarkerLowGoo:
                break;
            default:
                return;
            }
            break;

        case kMarkerUpWater:
        case kMarkerUpLink:
        case kMarkerUpStack:
        case kMarkerUpGoo:
            switch (pSprite2->type) {
            case kMarkerUpWater:
            case kMarkerUpLink:
            case kMarkerUpStack:
            case kMarkerUpGoo:
                break;
            default:
                return;
            }
            break;
        }

        // swap link location
        /*short tmp1 = pXSprite2.data1;*/
        /*pXSprite2.data1 = pXSprite.data1;*/
        /*pXSprite.data1 = tmp1;*/

        if (pXSprite->data2 < kMaxPAL && pXSprite2->data2 < kMaxPAL)
        {
            // swap medium
            int tmp2 = pXSprite2->data2;
            pXSprite2->data2 = pXSprite->data2;
            pXSprite->data2 = tmp2;
        }


        // swap link type                       // swap link owners (sectors)
        short tmp3 = pSprite2->type;			//short tmp7 = pSprite2.owner;
        pSprite2->type = pSprite->type;			//pSprite2.owner = pSprite.owner;
        pSprite->type = tmp3;					//pSprite.owner = tmp7;

        // Deal with linked sectors
        sectortype *pSector = &sector[pSprite->sectnum];
        sectortype *pSector2 = &sector[pSprite2->sectnum];

        // Check for underwater
        XSECTOR *pXSector = NULL;	XSECTOR *pXSector2 = NULL;
        if (pSector->extra > 0) pXSector = &xsector[pSector->extra];
        if (pSector2->extra > 0) pXSector2 = &xsector[pSector2->extra];
        if (pXSector != NULL && pXSector2 != NULL) {
            bool tmp6 = pXSector->Underwater;
            pXSector->Underwater = pXSector2->Underwater;
            pXSector2->Underwater = tmp6;
        }

        // optionally swap floorpic
        if (pXSprite2->data3 == 1) {
            short tmp4 = pSector->floorpicnum;
            pSector->floorpicnum = pSector2->floorpicnum;
            pSector2->floorpicnum = tmp4;
        }

        // optionally swap ceilpic
        if (pXSprite2->data4 == 1) {
            short tmp5 = pSector->ceilingpicnum;
            pSector->ceilingpicnum = pSector2->ceilingpicnum;
            pSector2->ceilingpicnum = tmp5;
        }
    }
    break;
    // By NoOne: add a way to link between path markers, so path sectors can change their path on the fly.
    case kMarkerPath:
    {
        // only path marker to path marker link allowed
        if (a3.type == 3)
        {
            int nXSprite2 = sprite[a3.index].extra;
            // get master path marker data fields
            pXSprite->data1 = xsprite[nXSprite2].data1;
            pXSprite->data2 = xsprite[nXSprite2].data2;
            pXSprite->data3 = xsprite[nXSprite2].data3; // include soundId(?)

            // get master path marker busy and wait times
            pXSprite->busyTime = xsprite[nXSprite2].busyTime;
            pXSprite->waitTime = xsprite[nXSprite2].waitTime;

        }
    }
    break;
    case kSwitchCombo:
    {
        if (a3.type == 3)
        {
            int nSprite2 = a3.index;
            int nXSprite2 = sprite[nSprite2].extra;
            dassert(nXSprite2 > 0 && nXSprite2 < kMaxXSprites);
            pXSprite->data1 = xsprite[nXSprite2].data1;
            if (pXSprite->data1 == pXSprite->data2)
                SetSpriteState(nSprite, pXSprite, 1);
            else
                SetSpriteState(nSprite, pXSprite, 0);
        }
    }
    break;
    default:
    {
        pXSprite->busy = nBusy;
        if ((pXSprite->busy & 0xffff) == 0)
            SetSpriteState(nSprite, pXSprite, nBusy >> 16);
    }
    break;
    }
}

void LinkWall(int nWall, XWALL *pXWall, EVENT a3)
{
    int nBusy = GetSourceBusy(a3);
    pXWall->busy = nBusy;
    if ((pXWall->busy & 0xffff) == 0)
        SetWallState(nWall, pXWall, nBusy>>16);
}

void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int a3)
{
    dassert(nSector < (unsigned int)numsectors);
    if (!pXSector->locked && !pXSector->at16_6)
    {
        if (pXSector->triggerOnce)
            pXSector->at16_6 = 1;
        if (pXSector->decoupled)
        {
            if (pXSector->txID)
                evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        }
        else
        {
            EVENT evnt;
            evnt.cmd = a3;
            OperateSector(nSector, pXSector, evnt);
        }
    }
}

void trMessageSector(unsigned int nSector, EVENT a2)
{
    dassert(nSector < (unsigned int)numsectors);
    dassert(sector[nSector].extra > 0 && sector[nSector].extra < kMaxXSectors);
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = &xsector[nXSector];
    if (!pXSector->locked || a2.cmd == 7 || a2.cmd == 8)
    {
        if (a2.cmd == 5)
            LinkSector(nSector, pXSector, a2);
        else if (a2.cmd == kGDXCommandPaste)
            pastePropertiesInObj(6, nSector, a2);
        else
            OperateSector(nSector, pXSector, a2);
    }
}

void trTriggerWall(unsigned int nWall, XWALL *pXWall, int a3)
{
    dassert(nWall < (unsigned int)numwalls);
    if (!pXWall->locked && !pXWall->isTriggered)
    {
        if (pXWall->triggerOnce)
            pXWall->isTriggered = 1;
        if (pXWall->decoupled)
        {
            if (pXWall->txID)
                evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
        }
        else
        {
            EVENT evnt;
            evnt.cmd = a3;
            OperateWall(nWall, pXWall, evnt);
        }
    }
}

void trMessageWall(unsigned int nWall, EVENT a2)
{
    dassert(nWall < (unsigned int)numwalls);
    dassert(wall[nWall].extra > 0 && wall[nWall].extra < kMaxXWalls);
    int nXWall = wall[nWall].extra;
    XWALL *pXWall = &xwall[nXWall];
    if (!pXWall->locked || a2.cmd == 7 || a2.cmd == 8)
    {
        if (a2.cmd == 5)
            LinkWall(nWall, pXWall, a2);
        else if (a2.cmd == kGDXCommandPaste)
            pastePropertiesInObj(0, nWall, a2);
        else
            OperateWall(nWall, pXWall, a2);
    }
}

void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int a3)
{
    if (!pXSprite->locked && !pXSprite->isTriggered)
    {
        if (pXSprite->triggerOnce)
            pXSprite->isTriggered = 1;
        if (pXSprite->Decoupled)
        {
            if (pXSprite->txID)
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
        }
        else
        {
            EVENT evnt;
            evnt.cmd = a3;
            OperateSprite(nSprite, pXSprite, evnt);
        }
    }
}

void trMessageSprite(unsigned int nSprite, EVENT a2)
{
    if (sprite[nSprite].statnum == kStatFree)
        return;
    int nXSprite = sprite[nSprite].extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    if (!pXSprite->locked || a2.cmd == 7 || a2.cmd == 8)
    {
        if (a2.cmd == 5)
            LinkSprite(nSprite, pXSprite, a2);
        else if (a2.cmd == kGDXCommandPaste)
            pastePropertiesInObj(3, nSprite, a2);
        else if (a2.cmd == kGDXCommandSpriteDamage)
            trDamageSprite(3, nSprite, a2);
        else
            OperateSprite(nSprite, pXSprite, a2);
    }
}

// By NoOne: this function damages sprite
void trDamageSprite(int type, int nDest, EVENT event) {
    UNREFERENCED_PARAMETER(type);

    /* - damages xsprite via TX ID	- */
    /* - data3 = damage type		- */
    /* - data4 = damage amount		- */

    if (event.type == 3) {
        spritetype* pSource = NULL; pSource = &sprite[event.index];
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if (xsprite[sprite[nDest].extra].health > 0) {
            if (pXSource->data4 == 0)
                pXSource->data4 = 65535;

            int dmgType = pXSource->data3;
            if (pXSource->data3 >= 7)
                dmgType = Random(6);

            actDamageSprite(pSource->xvel, &sprite[nDest], (DAMAGE_TYPE) dmgType, pXSource->data4);
        }
    }
}

bool valueIsBetween(int val, int min, int max) {
    return (val > min && val < max);
}
// By NoOne: this function used by various new GDX types.
void pastePropertiesInObj(int type, int nDest, EVENT event) {
    spritetype* pSource = NULL; pSource = &sprite[event.index];
    if (pSource == NULL || event.type != 3) return;
    XSPRITE* pXSource = &xsprite[pSource->extra];
    
    if (pSource->type == kGDXEffectSpawner) {
        /* - Effect Spawner can spawn any effect passed in data2 on it's or txID sprite - */
        if (pXSource->data2 < 0 || pXSource->data2 >= kFXMax) return;
        else if (type == 3)  useEffectGen(pXSource, &sprite[nDest]);
        return;

    } else if (pSource->type == kGDXSeqSpawner) {
        /* - SEQ Spawner takes data2 as SEQ ID and spawns it on it's or TX ID sprite - */
        if (pXSource->data2 <= 0 || !gSysRes.Lookup(pXSource->data2, "SEQ")) return;
        else if (type == 3) useSeqSpawnerGen(pXSource, &sprite[nDest]);
        return;

    } else if (pSource->type == kGDXObjDataAccumulator) {
        /* - Object Data Accumulator allows to perform sum and sub operations in data fields of object - */
        /* - data1 = destination data index 															- */
        /* - data2 = step value																			- */
        /* - data3 = min value																			- */
        /* - data4 = max value																			- */
        /* - min > max = sub, 	min < max = sum															- */

        /* - hitag: 0 = force OFF if goal value was reached for all objects		     					- */
        /* - hitag: 2 = force swap min and max if goal value was reached								- */
        /* - hitag: 3 = force reset counter	                                           					- */

        if (pXSource->data3 < 0) pXSource->data3 = 0;
        else if (pXSource->data3 > 32766) pXSource->data3 = 32767;
        if (pXSource->data4 < 0) pXSource->data4 = 0;
        else if (pXSource->data4 > 32766) pXSource->data4 = 32767;

        long data = getDataFieldOfObject(type, nDest, pXSource->data1);
        if (data == -65535) return;
        else if (pXSource->data3 < pXSource->data4) {

            if (data < pXSource->data3) data = pXSource->data3;
            if (data > pXSource->data4) data = pXSource->data4;

            if ((data += pXSource->data2) >= pXSource->data4) {
                switch (pSource->hitag) {
                    case 0:
                    case 1:
                        if (data > pXSource->data4) data = pXSource->data4;
                        break;
                    case 2:
                    {
                        
                        if (data > pXSource->data4) data = pXSource->data4;
                        if (!goalValueIsReached(pXSource)) break;
                        int tmp = pXSource->data4;
                        pXSource->data4 = pXSource->data3;
                        pXSource->data3 = tmp;
                        
                    }
                    break;
                    case 3:
                        if (data > pXSource->data4) data = pXSource->data3;
                        break;
                }
            }

        }
        else if (pXSource->data3 > pXSource->data4) {

            if (data > pXSource->data3) data = pXSource->data3;
            if (data < pXSource->data4) data = pXSource->data4;

            if ((data -= pXSource->data2) <= pXSource->data4) {
                switch (pSource->hitag) {
                    case 0:
                    case 1:
                        if (data < pXSource->data4) data = pXSource->data4;
                        break;
                    case 2:
                    {
                        if (data < pXSource->data4) data = pXSource->data4;
                        int tmp = pXSource->data4;
                        pXSource->data4 = pXSource->data3;
                        pXSource->data3 = tmp;
                        break;
                    }
                    case 3:
                        if (data < pXSource->data4) data = pXSource->data3;
                        break;
                }
            }
        }
        
        setDataValueOfObject(type, nDest, pXSource->data1, data);
        return;

    } else if (pSource->type == kGDXWindGenerator) {

        /* - Wind generator via TX or for current sector if TX ID not specified - */
        /* - sprite.ang = sector wind direction									- */
        /* - data1 = randomness settings										- */
        /* - 		 0: no randomness											- */
        /* - 		 1: randomize wind velocity in data2						- */
        /* - 		 2: randomize current generator sprite angle				- */
        /* - 		 3: randomize both wind velocity and sprite angle			- */
        /* - data2 = wind velocity												- */
        /* - data3 = enable panning according current wind speed and direction	- */
        /* - data4 = pan floor and ceiling settings								- */
        /* - 		 0: use sector pan settings									- */
        /* - 		 1: pan only floor											- */
        /* - 		 2: pan only ceiling										- */
        /* - 		 3: pan both												- */

        /* - hi-tag = 1: force windAlways and panAlways							- */
        
        if (pXSource->data2 < 0) return;
        else if (type == 6) useSectorWindGen(pXSource, &sector[nDest]);
        return;


    } else if (pSource->type == kGDXObjDataChanger) {

        /* - Data field changer via TX - */
        /* - data1 = sprite data1 / sector data / wall data	- */
        /* - data2 = sprite data2	- */
        /* - data3 = sprite data3	- */
        /* - data4 = sprite data4	- */

        switch (type) {
            // for sectors
            case 6:
            {
                XSECTOR* pXSector = &xsector[sector[nDest].extra];

                if (valueIsBetween(pXSource->data1, -1, 32767))
                    pXSector->data = pXSource->data1;

                break;
            }
            // for sprites
            case 3:
            {
                XSPRITE* pXSprite = &xsprite[sprite[nDest].extra];

                if (valueIsBetween(pXSource->data1, -1, 32767))
                    pXSprite->data1 = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    pXSprite->data2 = pXSource->data2;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    pXSprite->data3 = pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    pXSprite->data4 = pXSource->data4;

                break;
            }
            // for walls
            case 0:
            {
                XWALL* pXWall = &xwall[wall[nDest].extra];

                if (valueIsBetween(pXSource->data1, -1, 32767))
                    pXWall->data = pXSource->data1;

                break;
            }
        }

    } else if (pSource->type == kGDXSectorFXChanger) {

        /* - FX Wave changer for sector via TX - */
        /* - data1 = Wave 	- */
        /* - data2 = Amplitude	- */
        /* - data3 = Freq	- */
        /* - data4 = Phase	- */

        if (type == 6) {
            XSECTOR* pXSector = &xsector[sector[nDest].extra];
            if (valueIsBetween(pXSource->data1, -1, 32767))
                pXSector->wave = pXSource->data1;

            if (pXSource->data2 >= 0) {

                if (pXSource->data2 > 127) pXSector->amplitude = 127;
                else pXSector->amplitude = pXSource->data2;

            }
            else if (pXSource->data2 < -1) {

                if (pXSource->data2 < -127) pXSector->amplitude = -127;
                else pXSector->amplitude = pXSource->data2;

            }

            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if (pXSource->data3 > 255) pXSector->freq = 255;
                else pXSector->freq = pXSource->data3;
            }

            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                if (pXSource->data4 > 255) pXSector->phase = 255;
                else pXSector->phase = (short)pXSource->data4;
            }

            if ((pSource->hitag & kHitagExtBit) != 0)
                pXSector->shadeAlways = true;

        }

    } else if (pSource->type == kGDXDudeTargetChanger) {

        /* - Target changer for dudes via TX																		- */

        /* - data1 = target dude data1 value (can be zero)															- */
        /* 			 666: attack everyone, even if data1 id does not fit, except mates (if any)						- */
        /* - data2 = 0: AI deathmatch mode																			- */
        /*			 1: AI team deathmatch mode																		- */
        /* - data3 = 0: do not force target to fight dude back and *do not* awake some inactive monsters in sight	- */
        /* 			 1: force target to fight dude back	and *do not* awake some inactive monsters in sight			- */
        /*			 2: force target to fight dude back	and awake some inactive monsters in sight					- */
        /* - data4 = 0: do not ignore player(s) (even if enough targets in sight)									- */
        /*			 1: try to ignore player(s) (while enough targets in sight)										- */
        /*			 2: ignore player(s) (attack only when no targets in sight at all)								- */
        /*			 3: go to idle state if no targets in sight and ignore player(s) always							- */
        /*			 4: follow player(s) when no targets in sight, attack targets if any in sight					- */

        if (type != 3 || !IsDudeSprite(&sprite[nDest]) || sprite[nDest].statnum != 6) return;
        spritetype* pSprite = &sprite[nDest]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
        spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; int receiveHp = 33 + Random(33);
        DUDEINFO* pDudeInfo = &dudeInfo[pSprite->lotag - kDudeBase]; int matesPerEnemy = 1;

        // dude is burning?
        if (pXSprite->burnTime > 0 && pXSprite->burnSource >= 0 && pXSprite->burnSource < kMaxSprites) {
            if (!IsBurningDude(pSprite))
            {
                spritetype* pBurnSource = &sprite[pXSprite->burnSource];
                if (pBurnSource->extra >= 0) {
                    if (pXSource->data2 == 1 && isMateOf(pXSprite, &xsprite[pBurnSource->extra])) {
                        pXSprite->burnTime = 0;

                        // heal dude a bit in case of friendly fire
                        if (pXSprite->data4 > 0 && pXSprite->health < pXSprite->data4)
                            actHealDude(pXSprite, receiveHp, pXSprite->data4);
                        else if (pXSprite->health < pDudeInfo->startHealth)
                            actHealDude(pXSprite, receiveHp, pDudeInfo->startHealth);
                    }
                    else if (xsprite[pBurnSource->extra].health <= 0) {
                        pXSprite->burnTime = 0;
                    }
                }
            }
            else {
                actKillDude(pSource->xvel, pSprite, DAMAGE_TYPE_0, 65535);
                return;
            }
        }

        spritetype* pPlayer = targetIsPlayer(pXSprite);
        // special handling for player(s) if target changer data4 > 2.
        if (pPlayer != NULL) {
            if (pXSource->data4 == 3) {
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                aiSetGenIdleState(pSprite, pXSprite);
                if (pSprite->lotag == kGDXDudeUniversalCultist)
                    removeLeech(leechIsDropped(pSprite));
            }
            else if (pXSource->data4 == 4) {
                aiSetTarget(pXSprite, pPlayer->x, pPlayer->y, pPlayer->z);
                if (pSprite->lotag == kGDXDudeUniversalCultist)
                    removeLeech(leechIsDropped(pSprite));
            }
        }

        int maxAlarmDudes = 8 + Random(8);
        if (pXSprite->target > -1 && sprite[pXSprite->target].extra > -1 && pPlayer == NULL) {
            pTarget = &sprite[pXSprite->target]; pXTarget = &xsprite[pTarget->extra];

            if (unitCanFly(pSprite) && isMeleeUnit(pTarget) && !unitCanFly(pTarget))
                pSprite->hitag |= 0x0002;
            else if (unitCanFly(pSprite))
                pSprite->hitag &= ~0x0002;

            if (!IsDudeSprite(pTarget) || pXTarget->health < 1 || !dudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) {
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
            }
            // dude attack or attacked by target that does not fit by data id?
            else if (pXSource->data1 != 666 && pXTarget->data1 != pXSource->data1) {
                if (affectedByTargetChg(pXTarget)) {

                    // force stop attack target
                    aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                    if (pXSprite->burnSource == pTarget->xvel) {
                        pXSprite->burnTime = 0;
                        pXSprite->burnSource = -1;
                    }

                    // force stop attack dude
                    aiSetTarget(pXTarget, pTarget->x, pTarget->y, pTarget->z);
                    if (pXTarget->burnSource == pSprite->xvel) {
                        pXTarget->burnTime = 0;
                        pXTarget->burnSource = -1;
                    }
                }
                
            }
            // instantly kill annoying spiders, rats, hands etc if dude is big enough
            else if (isAnnoyingUnit(pTarget) && !isAnnoyingUnit(pSprite) && tilesiz[pSprite->picnum].y >= 60 &&
                getTargetDist(pSprite, pDudeInfo, pTarget) < 2) {

                actKillDude(pSource->xvel, pTarget, DAMAGE_TYPE_0, 65535);
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);

            }
            else if (pXSource->data2 == 1 && isMateOf(pXSprite, pXTarget)) {
                spritetype* pMate = pTarget; XSPRITE* pXMate = pXTarget;

                // heal dude
                if (pXSprite->data4 > 0 && pXSprite->health < pXSprite->data4)
                    actHealDude(pXSprite, receiveHp, pXSprite->data4);
                else if (pXSprite->health < pDudeInfo->startHealth)
                    actHealDude(pXSprite, receiveHp, pDudeInfo->startHealth);

                // heal mate
                if (pXMate->data4 > 0 && pXMate->health < pXMate->data4)
                    actHealDude(pXMate, receiveHp, pXMate->data4);
                else {
                    DUDEINFO* pTDudeInfo = &dudeInfo[pMate->lotag - kDudeBase];
                    if (pXMate->health < pTDudeInfo->startHealth)
                        actHealDude(pXMate, receiveHp, pTDudeInfo->startHealth);
                }

                if (pXMate->target > -1 && sprite[pXMate->target].extra >= 0) {
                    pTarget = &sprite[pXMate->target];
                    // force mate stop attack dude, if he does
                    if (pXMate->target == pSprite->xvel) {
                        aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                    }
                    else if (!isMateOf(pXSprite, &xsprite[pTarget->extra])) {
                        // force dude to attack same target that mate have
                        aiSetTarget(pXSprite, pTarget->xvel);
                        return;

                    }
                    else {
                        // force mate to stop attack another mate
                        aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                    }
                }

                // force dude stop attack mate, if target was not changed previously
                if (pXSprite->target == pMate->xvel)
                    aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);

                
            }
            // check if targets aims player then force this target to fight with dude
            else if (targetIsPlayer(pXTarget) != NULL) {
                aiSetTarget(pXTarget, pSprite->xvel);
            }

            int mDist = 3; if (isMeleeUnit(pSprite)) mDist = 2;
            if (pXSprite->target >= 0 && getTargetDist(pSprite, pDudeInfo, &sprite[pXSprite->target]) < mDist) {
                if (!isActive(pSprite->xvel)) aiActivateDude(pSprite, pXSprite);
                return;
            }
            // lets try to look for target that fits better by distance
            else if ((gFrameClock & 256) != 0 && (pXSprite->target < 0 || getTargetDist(pSprite, pDudeInfo, pTarget) >= mDist)) {
                pTarget = getTargetInRange(pSprite, 0, mDist, pXSource->data1, pXSource->data2);
                if (pTarget != NULL) {
                    pXTarget = &xsprite[pTarget->extra];

                    // Make prev target not aim in dude
                    if (pXSprite->target > -1) {
                        spritetype* prvTarget = &sprite[pXSprite->target];
                        aiSetTarget(&xsprite[prvTarget->extra], prvTarget->x, prvTarget->y, prvTarget->z);
                        if (!isActive(pTarget->xvel))
                            aiActivateDude(pTarget, pXTarget);
                    }

                    // Change target for dude
                    aiSetTarget(pXSprite, pTarget->xvel);
                    if (!isActive(pSprite->xvel))
                        aiActivateDude(pSprite, pXSprite);

                    // ...and change target of target to dude to force it fight
                    if (pXSource->data3 > 0 && pXTarget->target != pSprite->xvel) {
                        aiSetTarget(pXTarget, pSprite->xvel);
                        if (!isActive(pTarget->xvel))
                            aiActivateDude(pTarget, pXTarget);
                    }
                    return;
                }
            }
        }

        if ((pXSprite->target < 0 || pPlayer != NULL) && (gFrameClock & 32) != 0) {
            // try find first target that dude can see
            for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
                pTarget = &sprite[nSprite]; pXTarget = &xsprite[pTarget->extra];

                if (pXTarget->target == pSprite->xvel) {
                    aiSetTarget(pXSprite, pTarget->xvel);
                    return;
                }

                // skip non-dudes and players
                if (!IsDudeSprite(pTarget) || (IsPlayerSprite(pTarget) && pXSource->data4 > 0) || pTarget->owner == pSprite->xvel) continue;
                // avoid self aiming, those who dude can't see, and those who dude own
                else if (!dudeCanSeeTarget(pXSprite, pDudeInfo, pTarget) || pSprite->xvel == pTarget->xvel) continue;
                // if Target Changer have data1 = 666, everyone can be target, except AI team mates.
                else if (pXSource->data1 != 666 && pXSource->data1 != pXTarget->data1) continue;
                // don't attack immortal, burning dudes and mates
                if (IsBurningDude(pTarget) || !IsKillableDude(pTarget, true) || (pXSource->data2 == 1 && isMateOf(pXSprite, pXTarget)))
                    continue;

                if (pXSource->data2 == 0 || (pXSource->data2 == 1 && !isMatesHaveSameTarget(pXSprite, pTarget, matesPerEnemy))) {

                    // Change target for dude
                    aiSetTarget(pXSprite, pTarget->xvel);
                    if (!isActive(pSprite->xvel))
                        aiActivateDude(pSprite, pXSprite);

                    // ...and change target of target to dude to force it fight
                    if (pXSource->data3 > 0 && pXTarget->target != pSprite->xvel) {
                        aiSetTarget(pXTarget, pSprite->xvel);
                        if (!isActive(pTarget->xvel))
                            aiActivateDude(pTarget, pXTarget);

                        if (pXSource->data3 == 2)
                            disturbDudesInSight(pTarget, maxAlarmDudes);
                    }
                    return;
                }
                break;
            }
        }

        // got no target - let's ask mates if they have targets
        if ((pXSprite->target < 0 || pPlayer != NULL) && pXSource->data2 == 1 && (gFrameClock & 64) != 0) {
            spritetype* pMateTarget = NULL;
            if ((pMateTarget = getMateTargets(pXSprite)) != NULL && pMateTarget->extra > 0) {
                XSPRITE* pXMateTarget = &xsprite[pMateTarget->extra];
                if (dudeCanSeeTarget(pXSprite, pDudeInfo, pMateTarget)) {
                    if (pXMateTarget->target < 0) {
                        aiSetTarget(pXMateTarget, pSprite->xvel);
                        if (IsDudeSprite(pMateTarget) && !isActive(pMateTarget->xvel))
                            aiActivateDude(pMateTarget, pXMateTarget);
                    }

                    aiSetTarget(pXSprite, pMateTarget->xvel);
                    if (!isActive(pSprite->xvel))
                        aiActivateDude(pSprite, pXSprite);
                    return;

                    // try walk in mate direction in case if not see the target
                }
                else if (pXMateTarget->target >= 0 && dudeCanSeeTarget(pXSprite, pDudeInfo, &sprite[pXMateTarget->target])) {
                    spritetype* pMate = &sprite[pXMateTarget->target];
                    pXSprite->target = pMateTarget->xvel;
                    pXSprite->targetX = pMate->x;
                    pXSprite->targetY = pMate->y;
                    pXSprite->targetZ = pMate->z;
                    if (!isActive(pSprite->xvel))
                        aiActivateDude(pSprite, pXSprite);
                    return;
                }
            }
        }

    } else if (pSource->type == kGDXObjSizeChanger) {

        /* - size and pan changer of sprite/wall/sector via TX ID 	- */
        /* - data1 = sprite xrepeat / wall xrepeat / floor xpan 	- */
        /* - data2 = sprite yrepeat / wall yrepeat / floor ypan 	- */
        /* - data3 = sprite xoffset / wall xoffset / ceil xpan 		- */
        /* - data3 = sprite yoffset / wall yoffset / ceil ypan 		- */

        if (pXSource->data1 > 255) pXSource->data1 = 255;
        if (pXSource->data2 > 255) pXSource->data2 = 255;
        if (pXSource->data3 > 255) pXSource->data3 = 255;
        if (valueIsBetween(pXSource->data4, 255, 65535))
            pXSource->data4 = 255;

        switch (type) {
            // for sectors
            case 6:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    sector[nDest].floorxpanning = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    sector[nDest].floorypanning = pXSource->data2;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    sector[nDest].ceilingxpanning = pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    sector[nDest].ceilingypanning = (short)pXSource->data4;
                break;
            // for sprites
            case 3:

                if (valueIsBetween(pXSource->data1, -1, 32767) &&
                    valueIsBetween(pXSource->data2, -1, 32767) &&
                    pXSource->data1 < 4 && pXSource->data2 < 4)
                {

                    sprite[nDest].xrepeat = 4;
                    sprite[nDest].yrepeat = 4;
                    sprite[nDest].cstat |= kSprInvisible;

                }
                else {

                    if (valueIsBetween(pXSource->data1, -1, 32767)) {
                        if (pXSource->data1 < 4)
                            sprite[nDest].xrepeat = 4;
                        else
                            sprite[nDest].xrepeat = pXSource->data1;
                    }

                    if (valueIsBetween(pXSource->data2, -1, 32767)) {
                        if (pXSource->data2 < 4)
                            sprite[nDest].yrepeat = 4;
                        else
                            sprite[nDest].yrepeat = pXSource->data2;
                    }
                }

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    sprite[nDest].xoffset = pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    sprite[nDest].yoffset = (short)pXSource->data4;

                break;
            // for walls
            case 0:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    wall[nDest].xrepeat = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    wall[nDest].yrepeat = pXSource->data2;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    wall[nDest].xpanning = (short)pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    wall[nDest].ypanning = (short)pXSource->data4;

                break;
        }

    } else if (pSource->type == kGDXObjPicnumChanger) {

        /* - picnum changer can change picnum of sprite/wall/sector via TX ID - */
        /* - data1 = sprite pic / wall pic / sector floor pic 				 - */
        /* - data3 = sprite pal / wall pal / sector floor pic				- */

        switch (type) {
            // for sectors
            case 6:
            {
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    sector[nDest].floorpicnum = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    sector[nDest].ceilingpicnum = pXSource->data2;

                XSECTOR *pXSector = &xsector[sector[nDest].extra];
                if (valueIsBetween(pXSource->data3, -1, 32767)) {
                    sector[nDest].floorpal = pXSource->data3;
                    if ((pSource->hitag & kHitagExtBit) != 0)
                        pXSector->floorpal = pXSource->data3;
                }

                if (valueIsBetween(pXSource->data4, -1, 65535)) {
                    sector[nDest].ceilingpal = (short)pXSource->data4;
                    if ((pSource->hitag & kHitagExtBit) != 0)
                        pXSector->ceilpal = (short)pXSource->data4;
                }
                break;
            }
            // for sprites
            case 3:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    sprite[nDest].picnum = pXSource->data1;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    sprite[nDest].pal = pXSource->data3;
                break;
            // for walls
            case 0:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    wall[nDest].picnum = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    wall[nDest].overpicnum = pXSource->data2;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    wall[nDest].pal = pXSource->data3;
                break;
        }

    } else if (pSource->type == kGDXObjPropertiesChanger) {

        /* - properties changer can change various properties of sprite/wall/sector via TX ID 	- */
        /* - data1 = sector underwater status													- */
        /* - data2 = sector visibility 															- */
        /* - data3 = sector ceiling cstat / sprite / wall hitag 								- */
        /* - data4 = sector floor / sprite / wall cstat 										- */

        switch (type) {
        // for sectors
        case 6:
        {
            XSECTOR* pXSector = &xsector[sector[nDest].extra];

            switch (pXSource->data1) {
            case 0:
                pXSector->Underwater = false;
                break;
            case 1:
                pXSector->Underwater = true;
                break;
            case 2:
                pXSector->Depth = 0;
                break;
            case 3:
                pXSector->Depth = 1;
                break;
            case 4:
                pXSector->Depth = 2;
                break;
            case 5:
                pXSector->Depth = 3;
                break;
            case 6:
                pXSector->Depth = 4;
                break;
            case 7:
                pXSector->Depth = 5;
                break;
            case 8:
                pXSector->Depth = 6;
                break;
            case 9:
                pXSector->Depth = 7;
                break;
            }

            if (valueIsBetween(pXSource->data2, -1, 32767)) {
                if (pXSource->data2 > 255) sector[nDest].visibility = 255;
                else sector[nDest].visibility = pXSource->data2;
            }

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sector[nDest].ceilingstat = pXSource->data3;

            if (valueIsBetween(pXSource->data4, -1, 65535))
                sector[nDest].floorstat = pXSource->data4;
            break;
        }
        // for sprites
        case 3:
            if (valueIsBetween(pXSource->data3, -1, 32767))
                sprite[nDest].hitag = pXSource->data3;

            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                pXSource->data4 |= kSprOriginAlign;
                sprite[nDest].cstat = pXSource->data4;
            }
            break;
            // for walls
        case 0:
            if (valueIsBetween(pXSource->data3, -1, 32767))
                wall[nDest].hitag = pXSource->data3;

            if (valueIsBetween(pXSource->data4, -1, 65535))
                wall[nDest].cstat = pXSource->data4;
            break;
        }
    }
}
// By NoOne: the following functions required for kGDXDudeTargetChanger
//---------------------------------------
spritetype* getTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode) {
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; spritetype* cTarget = NULL;
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pTarget = &sprite[nSprite];  pXTarget = &xsprite[pTarget->extra];
        if (!dudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) continue;

        int dist = getTargetDist(pSprite, pDudeInfo, pTarget);
        if (dist < minDist || dist > maxDist) continue;
        else if (pXSprite->target == pTarget->xvel) return pTarget;
        else if (!IsDudeSprite(pTarget) || pTarget->xvel == pSprite->xvel || IsPlayerSprite(pTarget)) continue;
        else if (IsBurningDude(pTarget) || !IsKillableDude(pTarget, true) || pTarget->owner == pSprite->xvel) continue;
        else if ((teamMode == 1 && isMateOf(pXSprite, pXTarget)) || isMatesHaveSameTarget(pXSprite,pTarget,1)) continue;
        else if (data == 666 || pXTarget->data1 == data) {

            if (pXSprite->target > 0) {
                cTarget = &sprite[pXSprite->target];
                int fineDist1 = getFineTargetDist(pSprite, cTarget);
                int fineDist2 = getFineTargetDist(pSprite, pTarget);
                if (fineDist1 < fineDist2)
                    continue;
            }
            return pTarget;
        }
    }

    return NULL;
}

bool isMateOf(XSPRITE* pXDude, XSPRITE* pXSprite) {
    return (pXDude->rxID == pXSprite->rxID);
}

spritetype* targetIsPlayer(XSPRITE* pXSprite) {

    if (pXSprite->target >= 0) {
        if (IsPlayerSprite(&sprite[pXSprite->target]))
            return &sprite[pXSprite->target];
    }

    return NULL;
}

bool isTargetAimsDude(XSPRITE* pXTarget, spritetype* pDude) {
    return (pXTarget->target == pDude->xvel);
}

spritetype* getMateTargets(XSPRITE* pXSprite) {
    int rx = pXSprite->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type == 3) {
            pMate = &sprite[rxBucket[i].index];
            if (pMate->extra < 0 || pMate->xvel == sprite[pXSprite->reference].xvel || !IsDudeSprite(pMate))
                continue;

            pXMate = &xsprite[pMate->extra];
            if (pXMate->target > -1) {
                if (!IsPlayerSprite(&sprite[pXMate->target]))
                    return &sprite[pXMate->target];
            }

        }
    }

    return NULL;
}

bool isMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow) {
    int rx = pXLeader->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {

        if (rxBucket[i].type != 3)
            continue;

        pMate = &sprite[rxBucket[i].index];
        if (pMate->extra < 0 || pMate->xvel == sprite[pXLeader->reference].xvel || !IsDudeSprite(pMate))
            continue;

        pXMate = &xsprite[pMate->extra];
        if (pXMate->target == pTarget->xvel && allow-- <= 0)
            return true;
    }

    return false;

}

bool isActive(int nSprite) {
    spritetype* pDude = &sprite[nSprite]; XSPRITE* pXDude = &xsprite[pDude->extra];
    int stateType = pXDude->aiState->stateType;
    switch (stateType) {
        case kAiStateIdle:
        case kAiStateGenIdle:
        case kAiStateSearch:
        case kAiStateMove:
        case kAiStateOther:
            return false;
    }
    return true;
}

bool dudeCanSeeTarget(XSPRITE* pXDude, DUDEINFO* pDudeInfo, spritetype* pTarget) {
    spritetype* pDude = &sprite[pXDude->reference];
    int dx = pTarget->x - pDude->x; int dy = pTarget->y - pDude->y;
    
    // check target
    if (approxDist(dx, dy) < pDudeInfo->seeDist) {
        int eyeAboveZ = pDudeInfo->eyeHeight * pDude->yrepeat << 2;

        // is there a line of sight to the target?
        if (cansee(pDude->x, pDude->y, pDude->z, pDude->sectnum, pTarget->x, pTarget->y, pTarget->z - eyeAboveZ, pTarget->sectnum)) {
            /*int nAngle = getangle(dx, dy);
            int losAngle = ((1024 + nAngle - pDude->ang) & 2047) - 1024;

            // is the target visible?
            if (klabs(losAngle) < 2048) // 360 deg periphery here*/
                return true;
        }
    }

    return false;

}

// by NoOne: this function required if monsters in genIdle ai state. It wakes up monsters
// when kGDXDudeTargetChanger goes to off state, so they won't ignore the world.
void activateDudes(int rx) {
    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type != 3) continue;
        spritetype * pDude = &sprite[rxBucket[i].index]; XSPRITE * pXDude = &xsprite[pDude->extra];
        if (!IsDudeSprite(pDude) || pXDude->aiState->stateType != kAiStateGenIdle) continue;
            aiInitSprite(pDude);
    }
}

bool affectedByTargetChg(XSPRITE* pXDude) {
    if (pXDude->rxID <= 0 || pXDude->locked == 1) return false;
    for (int nSprite = headspritestat[kStatGDXDudeTargetChanger]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        XSPRITE* pXSprite = (sprite[nSprite].extra >= 0) ? &xsprite[sprite[nSprite].extra] : NULL;
        if (pXSprite == NULL || pXSprite->txID <= 0 || pXSprite->state != 1) continue;
        for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
            if (rxBucket[i].type != 3) continue;

            spritetype* pSprite = &sprite[rxBucket[i].index];
            if (pSprite->extra < 0 || !IsDudeSprite(pSprite)) continue;
            else if (pSprite->xvel == sprite[pXDude->reference].xvel) return true;
        }
    }
    return false;
}

int getDataFieldOfObject(int objType, int objIndex, int dataIndex) {
    int data = -65535;
    switch (objType) {
    case 3:
        switch (dataIndex) {
        case 1:
            return xsprite[sprite[objIndex].extra].data1;
        case 2:
            return xsprite[sprite[objIndex].extra].data2;
        case 3:
            return xsprite[sprite[objIndex].extra].data3;
        case 4:
            return xsprite[sprite[objIndex].extra].data4;
        default:
            return data;
        }
    case 0:
        return xsector[sector[objIndex].extra].data;
    case 6:
        return xwall[wall[objIndex].extra].data;
    default:
        return data;
    }
}

bool setDataValueOfObject(int objType, int objIndex, int dataIndex, int value) {
    switch (objType) {
    case 3:
        switch (dataIndex) {
        case 1:
            xsprite[sprite[objIndex].extra].data1 = value;
            return true;
        case 2:
            xsprite[sprite[objIndex].extra].data2 = value;
            return true;
        case 3:
            xsprite[sprite[objIndex].extra].data3 = value;
            return true;
        case 4:
            xsprite[sprite[objIndex].extra].data4 = value;
            return true;
        default:
            return false;
        }
    case 0:
        xsector[sector[objIndex].extra].data = value;
        return true;
    case 6:
        xwall[wall[objIndex].extra].data = value;
        return true;
    default:
        return false;
    }
}

// by NoOne: this function checks if all TX objects have the same value
bool goalValueIsReached(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, pXSprite->data1) != pXSprite->data4)
            return false;
    }
    return true;
}

// by NoOne: this function tells if there any dude found for kGDXDudeTargetChanger
bool getDudesForTargetChg(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != 3) continue;
        else if (IsDudeSprite(&sprite[rxBucket[i].index]) &&
            xsprite[sprite[rxBucket[i].index].extra].health > 0) return true;
    }

    return false;
}

void disturbDudesInSight(spritetype* pSprite, int max) {
    spritetype* pDude = NULL; XSPRITE* pXDude = NULL;
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->lotag - kDudeBase];
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pDude = &sprite[nSprite];
        if (pDude->xvel == pSprite->xvel || !IsDudeSprite(pDude) || pDude->extra < 0)
            continue;
        pXDude = &xsprite[pDude->extra];
        if (dudeCanSeeTarget(pXSprite, pDudeInfo, pDude)) {
            if (pXDude->target != -1 || pXDude->rxID > 0)
                continue;

            aiSetTarget(pXDude, pDude->x, pDude->y, pDude->z);
            aiActivateDude(pDude, pXDude);
            if (max-- < 1)
                break;
        }
    }
}

int getTargetDist(spritetype* pSprite, DUDEINFO* pDudeInfo, spritetype* pTarget) {
    int x = pTarget->x; int y = pTarget->y;
    int dx = x - pSprite->x; int dy = y - pSprite->y;

    int dist = approxDist(dx, dy);
    if (dist <= pDudeInfo->meleeDist) return 0;
    if (dist >= pDudeInfo->seeDist) return 13;
    if (dist <= pDudeInfo->seeDist / 12) return 1;
    if (dist <= pDudeInfo->seeDist / 11) return 2;
    if (dist <= pDudeInfo->seeDist / 10) return 3;
    if (dist <= pDudeInfo->seeDist / 9) return 4;
    if (dist <= pDudeInfo->seeDist / 8) return 5;
    if (dist <= pDudeInfo->seeDist / 7) return 6;
    if (dist <= pDudeInfo->seeDist / 6) return 7;
    if (dist <= pDudeInfo->seeDist / 5) return 8;
    if (dist <= pDudeInfo->seeDist / 4) return 9;
    if (dist <= pDudeInfo->seeDist / 3) return 10;
    if (dist <= pDudeInfo->seeDist / 2) return 11;
    return 12;
}

int getFineTargetDist(spritetype* pSprite, spritetype* pTarget) {
    int x = pTarget->x; int y = pTarget->y;
    int dx = x - pSprite->x; int dy = y - pSprite->y;

    int dist = approxDist(dx, dy);
    return dist;
}

bool IsBurningDude(spritetype* pSprite) {
    if (pSprite == NULL) return false;
    switch (pSprite->type) {
    case 239: // burning dude
    case 240: // cultist burning
    case 241: // axe zombie burning
    case 242: // fat zombie burning
    case 252: // tiny caleb burning
    case 253: // beast burning
    case kGDXGenDudeBurning:
        return true;
    }

    return false;
}

bool IsKillableDude(spritetype* pSprite, bool locked) {
    if (!IsDudeSprite(pSprite)) return false;
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->lotag - kDudeBase];

    // Optionally check if dude is locked
    if (locked && xsprite[pSprite->extra].locked == 1)
        return false;


    // Make sure damage shift is greater than 0;
    int a = 0;
    for (int i = 0; i <= 6; i++) {
        a += pDudeInfo->startDamage[i];
    }

    if (a == 0) return false;
    return true;
}

bool isAnnoyingUnit(spritetype* pDude) {
    switch (pDude->lotag) {
    case 212: // hand
    case 213: // brown spider
    case 214: // red spider
    case 215: // black spider
    case 216: // mother spider
    case 218: // eel
    case 219: // bat
    case 220: // rat
    case 222: // green tentacle
    case 224: // fire tentacle
    case 226: // mother tentacle
    case 223: // green pod
    case 225: // fire pod
        return true;
    default:
        return false;
    }
}

bool unitCanFly(spritetype* pDude) {
    switch (pDude->lotag) {
    case 219: // bat
    case 206: // gargoyle
    case 207: // stone gargoyle
    case 210: // phantasm
        return true;
    default:
        return false;
    }
}

bool isMeleeUnit(spritetype* pDude) {
    switch (pDude->lotag) {
    case 203: // axe zombie
    case 205: // earth zombie
    case 206: // gargoyle
    case 212: // hand
    case 213: // brown spider
    case 214: // red spider
    case 215: // black spider
    case 216: // mother spider
    case 217: // gill beast
    case 218: // eel
    case 219: // bat
    case 220: // rat
    case 222: // green tentacle
    case 224: // fire tentacle
    case 226: // mother tentacle
    case 244: // sleep zombie
    case 245: // innocent
    case 250: // tiny caleb
    case 251: // beast
        return true;
    case kGDXDudeUniversalCultist:
        return (pDude->extra >= 0 && dudeIsMelee(&xsprite[pDude->extra]));
    default:
        return false;
    }
}
//---------------------------------------

void ProcessMotion(void)
{
    sectortype *pSector;
    int nSector;
    for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
    {
        int nXSector = pSector->extra;
        if (nXSector <= 0)
            continue;
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->bobSpeed != 0)
        {
            if (pXSector->bobAlways)
                pXSector->bobTheta += pXSector->bobSpeed;
            else if (pXSector->busy == 0)
                continue;
            else
                pXSector->bobTheta += mulscale16(pXSector->bobSpeed, pXSector->busy);
            int vdi = mulscale30(Sin(pXSector->bobTheta), pXSector->bobZRange<<8);
            for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
            {
                spritetype *pSprite = &sprite[nSprite];
                if (pSprite->cstat&24576)
                {
                    viewBackupSpriteLoc(nSprite, pSprite);
                    pSprite->z += vdi;
                }
            }
            if (pXSector->bobFloor)
            {
                int floorZ = pSector->floorz;
                viewInterpolateSector(nSector, pSector);
                pSector->floorz = baseFloor[nSector]+vdi;
                for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    spritetype *pSprite = &sprite[nSprite];
                    if (pSprite->hitag&2)
                        pSprite->hitag |= 4;
                    else
                    {
                        int top, bottom;
                        GetSpriteExtents(pSprite, &top, &bottom);
                        if (bottom >= floorZ && (pSprite->cstat&48) == 0)
                        {
                            viewBackupSpriteLoc(nSprite, pSprite);
                            pSprite->z += vdi;
                        }
                    }
                }
            }
            if (pXSector->bobCeiling)
            {
                int ceilZ = pSector->ceilingz;
                viewInterpolateSector(nSector, pSector);
                pSector->ceilingz = baseCeil[nSector]+vdi;
                for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    spritetype *pSprite = &sprite[nSprite];
                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    if (top <= ceilZ && (pSprite->cstat&48) == 0)
                    {
                        viewBackupSpriteLoc(nSprite, pSprite);
                        pSprite->z += vdi;
                    }
                }
            }
        }
    }
}

void AlignSlopes(void)
{
    sectortype *pSector;
    int nSector;
    for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
    {
        if (qsector_filler[nSector])
        {
            walltype *pWall = &wall[pSector->wallptr+qsector_filler[nSector]];
            walltype *pWall2 = &wall[pWall->point2];
            int nNextSector = pWall->nextsector;
            if (nNextSector >= 0)
            {
                int x = (pWall->x+pWall2->x)/2;
                int y = (pWall->y+pWall2->y)/2;
                viewInterpolateSector(nSector, pSector);
                alignflorslope(nSector, x, y, getflorzofslope(nNextSector, x, y));
                alignceilslope(nSector, x, y, getceilzofslope(nNextSector, x, y));
            }
        }
    }
}

int(*gBusyProc[])(unsigned int, unsigned int) =
{
    VCrushBusy,
    VSpriteBusy,
    VDoorBusy,
    HDoorBusy,
    RDoorBusy,
    StepRotateBusy,
    GenSectorBusy,
    PathBusy
};

void trProcessBusy(void)
{
    memset(velFloor, 0, sizeof(velFloor));
    memset(velCeil, 0, sizeof(velCeil));
    for (int i = gBusyCount-1; i >= 0; i--)
    {
        int oldBusy = gBusy[i].at8;
        gBusy[i].at8 = ClipRange(oldBusy+gBusy[i].at4*4, 0, 65536);
        int nStatus = gBusyProc[gBusy[i].atc](gBusy[i].at0, gBusy[i].at8);
        switch (nStatus)
        {
        case 1:
            gBusy[i].at8 = oldBusy;
            break;
        case 2:
            gBusy[i].at8 = oldBusy;
            gBusy[i].at4 = -gBusy[i].at4;
            break;
        case 3:
            gBusy[i] = gBusy[--gBusyCount];
            break;
        }
    }
    ProcessMotion();
    AlignSlopes();
}

void InitGenerator(int);

void trInit(void)
{
    gBusyCount = 0;
    for (int i = 0; i < numwalls; i++)
    {
        baseWall[i].x = wall[i].x;
        baseWall[i].y = wall[i].y;
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kStatFree)
        {
            sprite[i].zvel = sprite[i].type;
            baseSprite[i].x = sprite[i].x;
            baseSprite[i].y = sprite[i].y;
            baseSprite[i].z = sprite[i].z;
        }
        else
            sprite[i].zvel = -1;
    }
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        dassert(nXWall < kMaxXWalls);
        if (nXWall > 0)
        {
            XWALL *pXWall = &xwall[nXWall];
            if (pXWall->state)
                pXWall->busy = 65536;
        }
    }
    dassert((numsectors >= 0) && (numsectors < kMaxSectors));
    for (int i = 0; i < numsectors; i++)
    {
        sectortype *pSector = &sector[i];
        baseFloor[i] = pSector->floorz;
        baseCeil[i] = pSector->ceilingz;
        int nXSector = pSector->extra;
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->state)
                pXSector->busy = 65536;
            switch (pSector->lotag)
            {
            case kSecCounter:
                //By NoOne: no need to trigger once it, instead lock so it can be unlocked and used again.
                //pXSector->triggerOnce = 1;
                evPost(i, 6, 0, CALLBACK_ID_12);
                break;
            case 600:
            case 602:
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            case 614:
            case 616:
            {
                spritetype *pSprite1 = &sprite[pXSector->at2c_0];
                spritetype *pSprite2 = &sprite[pXSector->at2e_0];
                TranslateSector(i, 0, -65536, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->lotag == 616);
                for (int j = 0; j < pSector->wallnum; j++)
                {
                    baseWall[pSector->wallptr+j].x = wall[pSector->wallptr+j].x;
                    baseWall[pSector->wallptr+j].y = wall[pSector->wallptr+j].y;
                }
                for (int nSprite = headspritesect[i]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    baseSprite[nSprite].x = sprite[nSprite].x;
                    baseSprite[nSprite].y = sprite[nSprite].y;
                    baseSprite[nSprite].z = sprite[nSprite].z;
                }
                TranslateSector(i, 0, pXSector->busy, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->lotag == 616);
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            }
            case 615:
            case 617:
            {
                spritetype *pSprite1 = &sprite[pXSector->at2c_0];
                TranslateSector(i, 0, -65536, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, 0, pSprite1->x, pSprite1->y, pSprite1->ang, pSector->lotag == 617);
                for (int j = 0; j < pSector->wallnum; j++)
                {
                    baseWall[pSector->wallptr+j].x = wall[pSector->wallptr+j].x;
                    baseWall[pSector->wallptr+j].y = wall[pSector->wallptr+j].y;
                }
                for (int nSprite = headspritesect[i]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    baseSprite[nSprite].x = sprite[nSprite].x;
                    baseSprite[nSprite].y = sprite[nSprite].y;
                    baseSprite[nSprite].z = sprite[nSprite].z;
                }
                TranslateSector(i, 0, pXSector->busy, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, 0, pSprite1->x, pSprite1->y, pSprite1->ang, pSector->lotag == 617);
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            }
            case 612:
                InitPath(i, pXSector);
                break;
            default:
                break;
            }
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        int nXSprite = sprite[i].extra;
        if (sprite[i].statnum < kStatFree && nXSprite > 0)
        {
            dassert(nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->state)
                pXSprite->busy = 65536;
            switch (sprite[i].type)
            {
            case 23:
                pXSprite->triggerOnce = 1;
                break;
            case kGDXSequentialTX:
                break;
            case kGDXSeqSpawner:
            case kGDXDudeTargetChanger:
            case kGDXEffectSpawner:
            case kGDXWindGenerator:
            case 700:
            case 701:
            case 702:
            case 703:
            case 704:
            case 705:
            case 706:
            case 707:
            case 708:
                InitGenerator(i);
                break;
            case 401:
            case kGDXThingTNTProx:
                pXSprite->Proximity = 1;
                break;
            case 414:
                if (pXSprite->state)
                    sprite[i].hitag |= 7;
                else
                    sprite[i].hitag &= ~7;
                break;
            }
            if (pXSprite->Vector)
                sprite[i].cstat |= 256;
            if (pXSprite->Push)
                sprite[i].cstat |= 4096;
        }
    }
    evSend(0, 0, 7, COMMAND_ID_1);
    if (gGameOptions.nGameType == 1)
        evSend(0, 0, 9, COMMAND_ID_1);
    else if (gGameOptions.nGameType == 2)
        evSend(0, 0, 8, COMMAND_ID_1);
    else if (gGameOptions.nGameType == 3)
    {
        evSend(0, 0, 8, COMMAND_ID_1);
        evSend(0, 0, 10, COMMAND_ID_1);
    }
}

void trTextOver(int nId)
{
    char *pzMessage = levelGetMessage(nId);
    if (pzMessage)
        viewSetMessage(pzMessage);
}

void InitGenerator(int nSprite)
{
    dassert(nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->statnum != kMaxStatus);
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    switch (sprite[nSprite].type)
    {
    // By NoOne: intialize GDX generators
    case kGDXDudeTargetChanger:
        pSprite->cstat &= ~kSprBlock;
        pSprite->cstat |= kSprInvisible;
        if (pXSprite->busyTime <= 0) pXSprite->busyTime = 5;
        if (pXSprite->state != pXSprite->restState)
            evPost(nSprite, 3, 0, COMMAND_ID_21);
        return;
    case kGDXObjDataAccumulator:
    case kGDXSeqSpawner:
    case kGDXEffectSpawner:
        pSprite->cstat &= ~kSprBlock;
        pSprite->cstat |= kSprInvisible;
        if (pXSprite->state != pXSprite->restState)
            evPost(nSprite, 3, 0, COMMAND_ID_21);
        return;
    case 700:
        pSprite->cstat &= ~kSprBlock;
        pSprite->cstat |= kSprInvisible;
        break;
    }
    if (pXSprite->state != pXSprite->restState && pXSprite->busyTime > 0)
        evPost(nSprite, 3, (120*(pXSprite->busyTime+Random2(pXSprite->data1)))/10, COMMAND_ID_21);
}

void ActivateGenerator(int nSprite)
{
    dassert(nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->statnum != kMaxStatus);
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    switch (pSprite->type) {
    case 701:
    {
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        actSpawnThing(pSprite->sectnum, pSprite->x, pSprite->y, bottom, 423);
        break;
    }
    case 702:
    {
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        actSpawnThing(pSprite->sectnum, pSprite->x, pSprite->y, bottom, 424);
        break;
    }
    case 708:
    {
        // By NoOne: allow custom pitch for sounds in SFX gen.
        int pitch = pXSprite->data4 << 1; if (pitch < 2000) pitch = 0;
        sfxPlay3DSoundCP(pSprite, pXSprite->data2, -1, 0, pitch);
        break;
    }
    case 703:
        switch (pXSprite->data2)
        {
        case 0:
            FireballTrapSeqCallback(3, nXSprite);
            break;
        case 1:
            seqSpawn(35, 3, nXSprite, nFireballTrapClient);
            break;
        case 2:
            seqSpawn(36, 3, nXSprite, nFireballTrapClient);
            break;
        }
        break;
    // By NoOne: EctoSkull gen can now fire any missile
    case 704:
        switch (pXSprite->data2)
        {
        case 0:
            UniMissileTrapSeqCallback(3, nXSprite);
            break;
        case 1:
            seqSpawn(35, 3, nXSprite, nUniMissileTrapClient);
            break;
        case 2:
            seqSpawn(36, 3, nXSprite, nUniMissileTrapClient);
            break;
        }
        break;
    case 706:
    {
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        gFX.fxSpawn(FX_23, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
        break;
    }
    case 707:
    {
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        gFX.fxSpawn(FX_26, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
        break;
    }
    }
}

void FireballTrapSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->cstat&32)
        actFireMissile(pSprite, 0, 0, 0, 0, (pSprite->cstat&8) ? 0x4000 : -0x4000, 305);
    else
        actFireMissile(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, 305);
}

// By NoOne: Callback for trap that can fire any missile specified in data3
void UniMissileTrapSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];

    int nMissile = 307;
    if (pXSprite->data3 >= kMissileBase && pXSprite->data3 < kMissileMax)
        nMissile = pXSprite->data3;
    else
        return;

    if (pSprite->cstat&32)
        actFireMissile(pSprite, 0, 0, 0, 0, (pSprite->cstat&8) ? 0x4000 : -0x4000, nMissile);
    else
        actFireMissile(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, nMissile);
}

void MGunFireSeqCallback(int, int nXSprite)
{
    int nSprite = xsprite[nXSprite].reference;
    spritetype *pSprite = &sprite[nSprite];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    if (pXSprite->data2 > 0 || pXSprite->data1 == 0)
    {
        if (pXSprite->data2 > 0)
        {
            pXSprite->data2--;
            if (pXSprite->data2 == 0)
                evPost(nSprite, 3, 1, COMMAND_ID_0);
        }
        int dx = (Cos(pSprite->ang)>>16)+Random2(1000);
        int dy = (Sin(pSprite->ang)>>16)+Random2(1000);
        int dz = Random2(1000);
        actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_2);
        sfxPlay3DSound(pSprite, 359, -1, 0);
    }
}

void MGunOpenSeqCallback(int, int nXSprite)
{
    seqSpawn(39, 3, nXSprite, nMGunFireClient);
}

class TriggersLoadSave : public LoadSave
{
public:
    virtual void Load();
    virtual void Save();
};

void TriggersLoadSave::Load()
{
    Read(&gBusyCount, sizeof(gBusyCount));
    Read(gBusy, sizeof(gBusy));
    Read(basePath, sizeof(basePath));
}

void TriggersLoadSave::Save()
{
    Write(&gBusyCount, sizeof(gBusyCount));
    Write(gBusy, sizeof(gBusy));
    Write(basePath, sizeof(basePath));
}

static TriggersLoadSave *myLoadSave;

void TriggersLoadSaveConstruct(void)
{
    myLoadSave = new TriggersLoadSave();
}
