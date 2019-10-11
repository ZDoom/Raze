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

#include <random>

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
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "triggers.h"
#include "trig.h"
#include "view.h"
#include "sectorfx.h"
#include "messages.h"

BEGIN_BLD_NS

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

char SetSpriteState(int nSprite, XSPRITE* pXSprite, int nState)
{
    if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
        return 0;
    pXSprite->busy = nState << 16;
    pXSprite->state = nState;
    evKill(nSprite, 3);
    if ((sprite[nSprite].flags & 16) != 0 && sprite[nSprite].inittype >= kDudeBase && sprite[nSprite].inittype < kDudeMax)
    {
        pXSprite->respawnPending = 3;
        evPost(nSprite, 3, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
        return 1;
    }
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);
    if (pXSprite->txID)
    {
        if (pXSprite->command != kCmdLink && pXSprite->triggerOn && pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
        if (pXSprite->command != kCmdLink && pXSprite->triggerOff && !pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
    }
    return 1;
}

char modernTypeSetSpriteState(int nSprite, XSPRITE *pXSprite, int nState)
{
    if ((pXSprite->busy&0xffff) == 0 && pXSprite->state == nState)
        return 0;
    pXSprite->busy = nState<<16;
    pXSprite->state = nState;
    evKill(nSprite, 3);
    if ((sprite[nSprite].flags & 16) != 0 && sprite[nSprite].inittype >= kDudeBase && sprite[nSprite].inittype < kDudeMax)
    {
        pXSprite->respawnPending = 3;
        evPost(nSprite, 3, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
        return 1;
    }
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime*120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);

    if (pXSprite->txID != 0 && ((pXSprite->triggerOn && pXSprite->state) || (pXSprite->triggerOff && !pXSprite->state))) {

        // by NoOne: Sending new command instead of link is *required*, because types above
        //are universal and can paste properties in different objects.
        switch (pXSprite->command) {
            case kCmdLink:
            case kCmdModernUse:
                evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // just send command to change properties
                return 1;
            case kCmdUnlock:
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // send normal command first
                evSend(nSprite, 3, pXSprite->txID, kCmdModernUse);  // then send command to change properties
                return 1;
            default:
                evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // send first command to change properties
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // then send normal command
                return 1;
        }

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
        evPost(nWall, 0, (pXWall->waitTime*120) / 10, pXWall->restState ? kCmdOn : kCmdOff);
    if (pXWall->txID)
    {
        if (pXWall->command != kCmdLink && pXWall->triggerOn && pXWall->state)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
        if (pXWall->command != kCmdLink && pXWall->triggerOff && !pXWall->state)
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
        if (pXSector->command != kCmdLink && pXSector->triggerOn && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        if (pXSector->stopOn)
        {
            pXSector->stopOn = 0;
            pXSector->stopOff = 0;
        }
        else if (pXSector->atf_6)
            evPost(nSector, 6, (pXSector->waitTimeA * 120) / 10, kCmdOff);
    }
    else
    {
        if (pXSector->command != kCmdLink && pXSector->triggerOff && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        if (pXSector->stopOff)
        {
            pXSector->stopOn = 0;
            pXSector->stopOff = 0;
        }
        else if (pXSector->atf_7)
            evPost(nSector, 6, (pXSector->waitTimeB * 120) / 10, kCmdOn);
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

void LifeLeechOperate(spritetype *pSprite, XSPRITE *pXSprite, EVENT event)
{
    switch (event.cmd) {
    case kCmdSpritePush:
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
    case kCmdSpriteProximity:
    {
        int nTarget = pXSprite->target;
        if (nTarget >= 0 && nTarget < kMaxSprites)
        {
            if (!pXSprite->stateTimer)
            {
                spritetype *pTarget = &sprite[nTarget];
                if (pTarget->statnum == kStatDude && !(pTarget->flags&32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites)
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
                        int nMissileType = kMissileLifeLeechAltNormal + (pXSprite->data3 ? 1 : 0);
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
                            evPost(pSprite->index, 3, t2, kCallbackLeechStateTimer);
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

void OperateSprite(int nSprite, XSPRITE *pXSprite, EVENT event)
{
    spritetype *pSprite = &sprite[nSprite];
    
    if (gModernMap) {
        switch (event.cmd) {
            case kCmdUnlock:
            case kCmdToggleLock:
                switch (pSprite->type) {
                    case kModernWindGenerator:
                        if (pXSprite->locked) stopWindOnSectors(pXSprite);
                        break;
                }
                break;
        }
    }

    switch (event.cmd) {
        case kCmdLock:
            pXSprite->locked = 1;
            return;
        case kCmdUnlock:
            pXSprite->locked = 0;
            return;
        case kCmdToggleLock:
            pXSprite->locked = pXSprite->locked ^ 1;
            return;
    }

    if (gModernMap) {
        switch (pSprite->type) {
            
            // add linking for path markers and stacks feature
            case kMarkerLowWater:
            case kMarkerUpWater:
            case kMarkerUpGoo:
            case kMarkerLowGoo:
            case kMarkerUpLink:
            case kMarkerLowLink:
            case kMarkerUpStack:
            case kMarkerLowStack:
            case kMarkerPath:
                switch (pXSprite->command) {
                case kCmdLink:
                    if (pXSprite->txID <= 0) return;
                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    return;
                }
                break; // go normal operate switch

            // Random Event Switch takes random data field and uses it as TX ID
            case kModernRandomTX: {
                std::default_random_engine rng; int tx = 0; int maxRetries = 10;
                // set range of TX ID if data2 and data3 is empty.
                if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

                    // data1 must be less than data4
                    if (pXSprite->data1 > pXSprite->data4) {
                        short tmp = pXSprite->data1;
                        pXSprite->data1 = (short)pXSprite->data4;
                        pXSprite->data4 = tmp;
                    }

                    int total = pXSprite->data4 - pXSprite->data1;
                    while (maxRetries > 0) {

                        // use true random only for single player mode
                        // otherwise use Blood's default one. In the future it maybe possible to make
                        // host send info to clients about what was generated.

                        if (gGameOptions.nGameType != 0 || VanillaMode() || DemoRecordStatus()) tx = Random(total) + pXSprite->data1;
                        else {
                            rng.seed(std::random_device()());
                            tx = (int)my_random(pXSprite->data1, pXSprite->data4);
                        }

                        if (tx != pXSprite->txID) break;
                        maxRetries--;
                    }

                } else {
                    while (maxRetries > 0) {
                        if ((tx = GetRandDataVal(NULL, pSprite)) > 0 && tx != pXSprite->txID) break;
                        maxRetries--;
                    }
                }

                if (tx > 0) {
                    pXSprite->txID = tx;
                    SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                }
            }
            return;
            
            // Sequential Switch takes values from data fields starting from data1 and uses it as TX ID
            case kModernSequentialTX: {
                bool range = false; int cnt = 3; int tx = 0;
                // set range of TX ID if data2 and data3 is empty.
                if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

                    // data1 must be less than data4
                    if (pXSprite->data1 > pXSprite->data4) {
                        short tmp = pXSprite->data1;
                        pXSprite->data1 = (short)pXSprite->data4;
                        pXSprite->data4 = tmp;
                    }

                    // force send command to all TX id in a range
                    if (pSprite->flags & kModernTypeFlag1) {
                        for (pXSprite->txID = pXSprite->data1; pXSprite->txID <= pXSprite->data4; pXSprite->txID++) {
                            if (pXSprite->txID > 0)
                                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                        }

                        pXSprite->txID = pXSprite->sysData1 = 0;
                        return;
                    }

                    // Make sure txIndex is correct as we store current index of TX ID here.
                    if (pXSprite->sysData1 < pXSprite->data1) pXSprite->sysData1 = pXSprite->data1;
                    else if (pXSprite->sysData1 > pXSprite->data4) pXSprite->sysData1 = pXSprite->data4;

                    range = true;

                } else {

                    // force send command to all TX id specified in data
                    if (pSprite->flags & kModernTypeFlag1) {
                        for (int i = 0; i <= 3; i++) {
                            if ((pXSprite->txID = GetDataVal(pSprite, i)) > 0)
                                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                        }

                        pXSprite->txID = pXSprite->sysData1 = 0;
                        return;
                    }

                    // Make sure txIndex is correct as we store current index of data field here.
                    if (pXSprite->sysData1 > 3) pXSprite->sysData1 = 0;
                    else if (pXSprite->sysData1 < 0) pXSprite->sysData1 = 3;

                }

                switch (event.cmd) {
                    case kCmdOff:
                        if (range == false) {
                            while (cnt-- >= 0) { // skip empty data fields
                                pXSprite->sysData1--;
                                if (pXSprite->sysData1 < 0) pXSprite->sysData1 = 3;
                                tx = GetDataVal(pSprite, pXSprite->sysData1);
                                if (tx < 0) ThrowError(" -- Current data index is negative");
                                if (tx > 0) break;
                                continue;
                            }
                        } else {
                            pXSprite->sysData1--;
                            if (pXSprite->sysData1 < pXSprite->data1) {
                                pXSprite->sysData1 = pXSprite->data4;
                            }
                            tx = pXSprite->sysData1;
                        }
                        break;
                    default:
                        if (range == false) {
                            while (cnt-- >= 0) { // skip empty data fields
                                if (pXSprite->sysData1 > 3) pXSprite->sysData1 = 0;
                                tx = GetDataVal(pSprite, pXSprite->sysData1);
                                if (tx < 0) ThrowError(" ++ Current data index is negative");
                                pXSprite->sysData1++;
                                if (tx > 0) break;
                                continue;
                            }
                        } else {
                            tx = pXSprite->sysData1;
                            if (pXSprite->sysData1 >= pXSprite->data4) {
                                pXSprite->sysData1 = pXSprite->data1;
                                break;
                            }
                            pXSprite->sysData1++;
                        }
                        break;
                }

                pXSprite->txID = tx;
                SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            }
            return;
            
            case kMarkerWarpDest:
                if (pXSprite->txID <= 0) {
                    if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                        useTeleportTarget(pXSprite, NULL);
                    return;
                }
                modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                return;
            
            case kModernSpriteDamager:
                if (pXSprite->txID <= 0) {
                    if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                        useSpriteDamager(pXSprite, NULL);
                    return;
                }
                modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                return;

            case kModernObjPropertiesChanger:
                if (pXSprite->txID <= 0) {
                    if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                        usePropertiesChanger(pXSprite, -1, -1);
                    return;
                }
                modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                return;

            case kModernObjPicnumChanger:
            case kModernObjSizeChanger:
            case kModernSectorFXChanger:
            case kModernObjDataChanger:
            case kModernConcussSprite:
                modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                return;
            
            case kModernCustomDudeSpawn:
                if (gGameOptions.nMonsterSettings && actSpawnCustomDude(pSprite, -1) != NULL)
                    gKillMgr.sub_263E0(1);
                return;
            
            case kModernSeqSpawner:
            case kModernEffectSpawner:
                switch (event.cmd) {
                    case kCmdOff:
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:
                        if (pXSprite->txID <= 0)
                            (pSprite->type == kModernSeqSpawner) ? useSeqSpawnerGen(pXSprite, 3, pSprite->xvel) : useEffectGen(pXSprite, NULL);
                        else {

                            switch (pXSprite->command) {
                                case kCmdLink:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // just send command to change properties
                                    break;
                                case kCmdUnlock:
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // send normal command first
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse);  // then send command to change properties
                                    break;
                                default:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // send first command to change properties
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // then send normal command
                                    break;
                            }

                        }

                        if (pXSprite->busyTime > 0)
                            evPost(nSprite, 3, ClipLow((int(pXSprite->busyTime) + Random2(pXSprite->data1)) * 120 / 10, 0), kCmdRepeat);
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                }
                return;

            case kModernWindGenerator:
                switch (event.cmd) {
                    case kCmdOff:
                        stopWindOnSectors(pXSprite);
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:
                        if (pXSprite->txID <= 0) useSectorWindGen(pXSprite, NULL);
                        else {

                            switch (pXSprite->command) {
                                case kCmdLink:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // just send command to change properties
                                    break;
                                case kCmdUnlock:
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // send normal command first
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse);  // then send command to change properties
                                    break;
                                default:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // send first command to change properties
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // then send normal command
                                    break;
                            }

                        }

                        if (pXSprite->busyTime > 0)  evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                }
                return;

            case kModernDudeTargetChanger: {
                
                // this one is required if data4 of generator was dynamically changed
                // it turns monsters in normal idle state instead of genIdle, so they
                // not ignore the world.
                bool activated = false;
                if (pXSprite->dropMsg == 3 && 3 != pXSprite->data4) {
                    activateDudes(pXSprite->txID);
                    activated = true;
                }

                switch (event.cmd) {
                    case kCmdOff:
                        if (pXSprite->data4 == 3 && activated == false) activateDudes(pXSprite->txID);
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:
                        if (pXSprite->txID <= 0 || !getDudesForTargetChg(pXSprite)) {
                            freeAllTargets(pXSprite);
                            evPost(nSprite, 3, 0, kCmdOff);
                            break;
                        }
                        else {

                            switch (pXSprite->command) {
                                case kCmdLink:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // just send command to change properties
                                    break;
                                case kCmdUnlock:
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // send normal command first
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse);  // then send command to change properties
                                    break;
                                default:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // send first command to change properties
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // then send normal command
                                    break;
                            }

                        }

                        if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    }

                    pXSprite->dropMsg = (short)pXSprite->data4;
            }
            return;

            case kModernObjDataAccumulator:
                switch (event.cmd) {
                    case kCmdOff:
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:

                        // force OFF after *all* TX objects reach the goal value
                        if (pSprite->flags == 0 && goalValueIsReached(pXSprite)) {
                            evPost(nSprite, 3, 0, kCmdOff);
                            break;
                        }

                        if (pXSprite->txID > 0 && pXSprite->data1 > 0 && pXSprite->data1 <= 4) {

                            switch (pXSprite->command) {
                                case kCmdLink:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // just send command to change properties
                                    break;
                                case kCmdUnlock:
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // send normal command first
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse);  // then send command to change properties
                                    break;
                                default:
                                    evSend(nSprite, 3, pXSprite->txID, kCmdModernUse); // send first command to change properties
                                    evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command); // then send normal command
                                    break;
                            }

                            if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                        }
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                }
                return;

            case kModernRandom:
            case kModernRandom2:
                switch (event.cmd) {
                    case kCmdOff:
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:
                        ActivateGenerator(nSprite);
                        if (pXSprite->busyTime > 0)
                            evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                }
                return;

            case kModernThingEnemyLifeLeech:
                dudeLeechOperate(pSprite, pXSprite, event);
                return;

            case kGenModernMissileUniversal:
                switch (event.cmd) {
                    case kCmdOff:
                        if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                        break;
                    case kCmdOn:
                        evKill(nSprite, 3); // queue overflow protect
                        if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                        fallthrough__;
                    case kCmdRepeat:
                        ActivateGenerator(nSprite);
                        if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                        if (pXSprite->busyTime > 0) evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                        break;
                    default:
                        if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                        else evPost(nSprite, 3, 0, kCmdOff);
                        break;
                }
                return;
        }
    } 
    
    if (pSprite->statnum == kStatDude && pSprite->type >= kDudeBase && pSprite->type < kDudeMax) {
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdSpriteProximity:
                if (pXSprite->state) break;
                fallthrough__;
            case kCmdOn:
            case kCmdSpritePush:
            case kCmdSpriteTouch:
                if (!pXSprite->state) SetSpriteState(nSprite, pXSprite, 1);
                aiActivateDude(pSprite, pXSprite);
                break;
            default:
                return;
        }
    }


    switch (pSprite->type) {
    case kTrapMachinegun:
        if (pXSprite->health <= 0) break; 
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0)) break;
                seqSpawn(40, 3, pSprite->extra, -1);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1)) break;
                seqSpawn(38, 3, pSprite->extra, nMGunOpenClient);
                if (pXSprite->data1 > 0)
                    pXSprite->data2 = pXSprite->data1;
                break;
        }
        break;
    case kThingFallingRock:
        if (SetSpriteState(nSprite, pXSprite, 1))
            pSprite->flags |= 7;
        break;
    case kThingWallCrack:
        if (SetSpriteState(nSprite, pXSprite, 0))
            actPostSprite(nSprite, kStatFree);
        break;
    case kThingCrateFace:
        if (SetSpriteState(nSprite, pXSprite, 0))
            actPostSprite(nSprite, kStatFree);
        break;
    case kTrapZapSwitchable:
        switch (event.cmd) {
            case kCmdOff:
                pXSprite->state = 0;
                pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                break;
            case kCmdOn:
                pXSprite->state = 1;
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat |= CSTAT_SPRITE_BLOCK;
                break;
            case kCmdToggle:
                pXSprite->state ^= 1;
                pSprite->cstat ^= CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat ^= CSTAT_SPRITE_BLOCK;
                break;
        }
        break;
    case kTrapFlame:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0)) break;
                seqSpawn(40, 3, pSprite->extra, -1);
                sfxKill3DSound(pSprite, 0, -1);
                break;
            case kCmdOn:
                if (SetSpriteState(nSprite, pXSprite, 1)) break;
                seqSpawn(38, 3, pSprite->extra, -1);
                sfxPlay3DSound(pSprite, 441, 0, 0);
                break;
        }
        break;
    case kSwitchPadlock:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1)) break;
                seqSpawn(37, 3, pSprite->extra, -1);
                break;
            default:
                SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                if (pXSprite->state) seqSpawn(37, 3, pSprite->extra, -1);
                break;
        }
        break;
    case kSwitchToggle:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0)) break;
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1)) break;
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)) break;
                if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
        }
        break;
    case kSwitchOneWay:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0)) break;
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1)) break;
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->restState ^ 1)) break;
                if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
        }
        break;
    case kSwitchCombo:
        switch (event.cmd) {
            case kCmdOff:
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
        
        sfxPlay3DSound(pSprite, pXSprite->data4, -1, 0);
        
        if (pXSprite->command == kCmdLink && pXSprite->txID > 0)
            evSend(nSprite, 3, pXSprite->txID, kCmdLink);

        if (pXSprite->data1 == pXSprite->data2) 
            SetSpriteState(nSprite, pXSprite, 1);
        else 
            SetSpriteState(nSprite, pXSprite, 0);

        break;
    case kMarkerDudeSpawn:
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
                switch (pXSprite->data1) {
                    case kDudeBurningInnocent:
                    case kDudeBurningCultist:
                    case kDudeBurningZombieButcher:
                    case kDudeBurningTinyCaleb:
                    case kDudeBurningBeast:
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
    case kMarkerEarthQuake:
        pXSprite->triggerOn = 0;
        pXSprite->isTriggered = 1;
        SetSpriteState(nSprite, pXSprite, 1);
        for (int p = connecthead; p >= 0; p = connectpoint2[p]) {
            spritetype *pPlayerSprite = gPlayer[p].pSprite;
            int dx = (pSprite->x - pPlayerSprite->x)>>4;
            int dy = (pSprite->y - pPlayerSprite->y)>>4;
            int dz = (pSprite->z - pPlayerSprite->z)>>8;
            int nDist = dx*dx+dy*dy+dz*dz+0x40000;
            gPlayer[p].at37f = divscale16(pXSprite->data1, nDist);
        }
        break;
    case kThingTNTBarrel:
        if (pSprite->flags&16) return;
        fallthrough__;
    case kThingArmedTNTStick:
    case kThingArmedTNTBundle:
    case kThingArmedSpray:
        actExplodeSprite(pSprite);
        break;
    case kTrapExploder:
        switch (event.cmd) {
            case kCmdOn:
                SetSpriteState(nSprite, pXSprite, 1);
                break;
            default:
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
                actExplodeSprite(pSprite);
                break;
        }
        break;
    case kThingArmedRemoteBomb:
        if (pSprite->statnum != kStatRespawn) {
            switch (event.cmd) {
                case kCmdOn:
                    actExplodeSprite(pSprite);
                    break;
                default:
                    sfxPlay3DSound(pSprite, 454, 0, 0);
                    evPost(nSprite, 3, 18, kCmdOff);
                    break;
            }
        }
        break;
    
    case kThingArmedProxBomb:
    case kModernThingTNTProx:
        if (pSprite->statnum != kStatRespawn) {
            switch (event.cmd) {
                case kCmdSpriteProximity:
                    if (pXSprite->state) break;
                    sfxPlay3DSound(pSprite, 452, 0, 0);
                    evPost(nSprite, 3, 30, kCmdOff);
                    pXSprite->state = 1;
                case kCmdOn:
                    sfxPlay3DSound(pSprite, 451, 0, 0);
                    pXSprite->Proximity = 1;
                    break;
                default:
                    actExplodeSprite(pSprite);
                    break;
            }
        }
        break;
    case kThingDroppedLifeLeech:
        LifeLeechOperate(pSprite, pXSprite, event);
        break;
    case kGenTrigger:
    case kGenDripWater:
    case kGenDripBlood:
    case kGenMissileFireball:
    case kGenMissileEctoSkull:
    case kGenDart:
    case kGenBubble:
    case kGenBubbleMulti:
    case kGenSound:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdRepeat:
                if (pSprite->type != kGenTrigger) ActivateGenerator(nSprite);
                if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                if (pXSprite->busyTime > 0) {
                    int nRand = Random2(pXSprite->data1);
                    evPost(nSprite, 3, 120*(nRand+pXSprite->busyTime) / 10, kCmdRepeat);
                }
                break;
            default:
                if (!pXSprite->state) {
                    SetSpriteState(nSprite, pXSprite, 1);
                    evPost(nSprite, 3, 0, kCmdRepeat);
                }
                break;
        }
        break;
    case kSoundPlayer:
        if (gGameOptions.nGameType != 0 || gMe->pXSprite->health <= 0) break;
        gMe->at30a = 0; sndStartSample(pXSprite->data1, -1, 1, 0);
        break;
    case kThingObjectGib:
    case kThingObjectExplode:
    case kThingBloodBits:
    case kThingBloodChunks:
    case kThingZombieHead:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
        }
        break;
    default:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdOn:
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
    spritetype* pSource = &sprite[pXSource->reference];
    
    if (pXSource->txID <= 0) {
        
        if (sector[pSource->sectnum].extra >= 0)
            xsector[sector[pSource->sectnum].extra].windVel = 0;

        return;
    }

    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type != 6) continue;
        XSECTOR * pXSector = &xsector[sector[rxBucket[i].index].extra];
        if ((pXSector->state == 1 && !pXSector->windAlways) || (sprite[pXSource->reference].flags & kModernTypeFlag1))
            pXSector->windVel = 0;
    }
}
/// WIP ////////////////////////////////////////////////////////
void useConcussSprite(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference];
    int nIndex = isDebris(pSprite->index);
    //ThrowError("%d", gPhysSpritesList[nIndex]);
    //int size = (tilesiz[pSprite->picnum].x * pSprite->xrepeat * tilesiz[pSprite->picnum].y * pSprite->yrepeat) >> 1;
    //int t = scale(pXSource->data1, size, gSpriteMass[pSprite->extra].mass);
    //xvel[pSprite->xvel] += mulscale16(t, pSprite->x);
    //yvel[pSprite->xvel] += mulscale16(t, pSprite->y);
    //zvel[pSprite->xvel] += mulscale16(t, pSprite->z);

    //debrisConcuss(pXSource->reference, nIndex, pSprite->x - 100, pSprite->y - 100, pSprite->z - 100, pXSource->data1);
}

void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex) {
    
    switch (objType) {
        
        // for walls
        case 0: {
            walltype* pWall = &wall[objIndex]; int old = -1;
            
            // data3 = set wall hitag
            if (valueIsBetween(pXSource->data3, -1, 32767))
                pWall->hitag = pXSource->data3;
        
            // data4 = set wall cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                old = pWall->cstat;
                pWall->cstat = pXSource->data4; // set new cstat

                // and hanlde exceptions
                if ((old & 0x2) && !(pWall->cstat & 0x2)) pWall->cstat |= 0x2; // kWallBottomSwap
                if ((old & 0x4) && !(pWall->cstat & 0x4)) pWall->cstat |= 0x4; // kWallBottomOrg, kWallOutsideOrg
                if ((old & 0x20) && !(pWall->cstat & 0x20)) pWall->cstat |= 0x20; // kWallOneWay

                if (old & 0xc000) {

                    if (!(pWall->cstat & 0xc000))
                        pWall->cstat |= 0xc000; // kWallMoveMask

                    if ((old & 0x0) && !(pWall->cstat & 0x0)) pWall->cstat |= 0x0; // kWallMoveNone
                    else if ((old & 0x4000) && !(pWall->cstat & 0x4000)) pWall->cstat |= 0x4000; // kWallMoveForward
                    else if ((old & 0x8000) && !(pWall->cstat & 0x8000)) pWall->cstat |= 0x8000; // kWallMoveBackward

                }
            }

            break;
        }
        
        // for sprites
        case 3: {
            spritetype* pSprite = &sprite[objIndex]; bool thing2debris = false;
            XSPRITE* pXSprite = &xsprite[pSprite->extra]; int old = -1;

            // data3 = set sprite hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                old = pSprite->hitag; pSprite->hitag = pXSource->data3; // set new hitag

                // and handle exceptions
                if ((old & kHitagFree) && !(pSprite->hitag & kHitagFree)) pSprite->hitag |= kHitagFree;
                if ((old & kHitagRespawn) && !(pSprite->hitag & kHitagRespawn)) pSprite->hitag |= kHitagRespawn;

                // prepare things for different (debris) physics.
                if (pSprite->statnum == kStatThing && debrisGetFreeIndex() >= 0) thing2debris = true;

            }

            // data2 = sprite physics settings
            if ((pXSource->data2 >= 0 && pXSource->data3 <= 33) || thing2debris) {
                switch (pSprite->statnum) {
                    case kStatDude: // dudes already treating in game
                    case kStatFree:
                    case kStatMarker:
                    case kStatPathMarker: // path marker
                        break;
                    default:
                        // store physics attributes in xsprite to avoid setting hitag for modern types!
                        int flags = (pXSprite->physAttr != 0) ? pXSprite->physAttr : 0;

                        if (thing2debris) {

                            // converting thing to debris
                            if ((pSprite->hitag & kPhysMove) != 0) flags |= kPhysMove;
                            else flags &= ~kPhysMove;

                            if ((pSprite->hitag & kPhysGravity) != 0) flags |= (kPhysGravity | kPhysFalling);
                            else flags &= ~(kPhysGravity | kPhysFalling);

                            pSprite->hitag &= ~(kPhysMove | kPhysGravity | kPhysFalling);
                            xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;  pXSprite->restState = pXSprite->state;

                        } else {

                            // first digit of data2: set main physics attributes
                            switch (pXSource->data2) {
                            case 0:
                                flags &= ~kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;

                            case 1: case 10: case 11: case 12: case 13:
                                flags |= kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;

                            case 2: case 20: case 21: case 22: case 23:
                                flags &= ~kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;

                            case 3: case 30: case 31: case 32: case 33:
                                flags |= kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                            }

                            // second digit of data2: set physics flags
                            switch (pXSource->data2) {
                            case 0: case 1: case 2: case 3:
                            case 10: case 20: case 30:
                                flags &= ~kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;

                            case 11: case 21: case 31:
                                flags |= kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;

                            case 12: case 22: case 32:
                                flags &= ~kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;

                            case 13: case 23: case 33:
                                flags |= kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                            }

                        }

                        int nIndex = isDebris(objIndex); // check if there is no sprite in list

                        // adding physics sprite in list
                        if ((flags & kPhysGravity) != 0 || (flags & kPhysMove) != 0) {
                        
							if (nIndex != -1) pXSprite->physAttr = flags; // just update physics attributes
							else if ((nIndex = debrisGetFreeIndex()) < 0)
								;// showWarning("Max (%d) Physics affected sprites reached!", kMaxSuperXSprites);
                            else {

                                pXSprite->physAttr = flags; // update physics attributes

                                // allow things to became debris, so they use different physics...
                                if (pSprite->statnum == kStatThing) changespritestat(objIndex, 0);
                                //actPostSprite(nDest, kStatDecoration); // !!!! not working here for some reason

                                gPhysSpritesList[nIndex] = objIndex;
                                if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
                                getSpriteMassBySize(pSprite); // create physics cache

                            }
                    
                        // removing physics from sprite in list (don't remove sprite from list)
                        } else if (nIndex != -1) {
                        
                            pXSprite->physAttr = flags;
                            xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;
                            if (pSprite->lotag >= kThingBase && pSprite->lotag < kThingMax)
                                changespritestat(objIndex, kStatThing);  // if it was a thing - restore statnum

                        }

                        break;
                }
            }

            // data4 = sprite cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {

                old = pSprite->cstat;
                pSprite->cstat = pXSource->data4; // set new cstat

                // and handle exceptions
                if ((old & 0x1000) && !(pSprite->cstat & 0x1000)) pSprite->cstat |= 0x1000; //kSpritePushable
                if ((old & 0x80) && !(pSprite->cstat & 0x80)) pSprite->cstat |= 0x80; // kSpriteOriginAlign

                if (old & 0x6000) {

                    if (!(pSprite->cstat & 0x6000))
                        pSprite->cstat |= 0x6000; // kSpriteMoveMask

                    if ((old & 0x0) && !(pSprite->cstat & 0x0)) pSprite->cstat |= 0x0; // kSpriteMoveNone
                    else if ((old & 0x2000) && !(pSprite->cstat & 0x2000)) pSprite->cstat |= 0x2000; // kSpriteMoveForward, kSpriteMoveFloor
                    else if ((old & 0x4000) && !(pSprite->cstat & 0x4000)) pSprite->cstat |= 0x4000; // kSpriteMoveReverse, kSpriteMoveCeiling

                }

            }

            break;
        }
        
        // for sectors
        case 6: {
            
            XSECTOR* pXSector = &xsector[sector[objIndex].extra];

            // data1 = sector underwater status and depth level
            if (pXSource->data1 == 0) pXSector->Underwater = false;
            else if (pXSource->data1 == 1) pXSector->Underwater = true;
            else if (pXSource->data1 > 9) pXSector->Depth = 7;
            else if (pXSource->data1 > 1) pXSector->Depth = pXSource->data1 - 2;

        
            // data2 = sector visibility
            if (valueIsBetween(pXSource->data2, -1, 32767)) {
                if (pXSource->data2 > 234) sector[objIndex].visibility = 234;
                else sector[objIndex].visibility = pXSource->data2;
            }

            // data3 = sector ceil cstat
            if (valueIsBetween(pXSource->data3, -1, 32767))
                sector[objIndex].ceilingstat = pXSource->data3;

            // data4 = sector floor cstat
            if (valueIsBetween(pXSource->data4, -1, 65535))
                sector[objIndex].floorstat = pXSource->data4;
        
            break;

        }
        
        // no TX id
        case -1: {
            
            // data2 = global visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                gVisibility = ClipRange(pXSource->data2, 0, 4096);
        }
        
        break;
    }

}

void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = (sector[pSource->sectnum].extra >= 0) ? &xsector[sector[pSource->sectnum].extra] : NULL;

    if (pSprite == NULL) {

        if (pXSource->data1 > 0) {
            for (int i = connecthead; i >= 0; i = connectpoint2[i]) {

                if (pXSource->data1 < kMaxPlayers) // relative to connected players
                    if (pXSource->data1 != (i + 1))
                        continue;
                else if (pXSource->data1 < (kDudePlayer1 + kMaxPlayers)) // absolute type
                    if (pXSource->data1 >= kDudePlayer1 && (pXSource->data1 + (kDudePlayer1 - 1)) == gPlayer[i].pSprite->type)
                        continue;

                useTeleportTarget(pXSource, gPlayer[i].pSprite);
                return;

            }
        }

        return;
    }

    pSprite->x = pSource->x; pSprite->y = pSource->y;
    pSprite->z += (sector[pSource->sectnum].floorz - sector[pSprite->sectnum].floorz);

    if (pSource->flags & kModernTypeFlag1) // force telefrag
        TeleFrag(pSprite->xvel, pSource->sectnum);

    changespritesect((short)pSprite->xvel, pSource->sectnum);
    if (pXSector != NULL && pXSector->Underwater) xsprite[pSprite->extra].medium = 1;
    else xsprite[pSprite->extra].medium = 0;

    if (pXSource->data2 == 1)
        pSprite->ang = pSource->ang;

    if (pXSource->data3 == 1)
        xvel[pSprite->xvel] = yvel[pSprite->xvel] = zvel[pSprite->xvel] = 0;

    viewBackupSpriteLoc(pSprite->xvel, pSprite);

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSource, pXSource->data4, -1, 0);

    if (IsPlayerSprite(pSprite)) {

        PLAYER* pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
        playerResetInertia(pPlayer);
        
        if (pXSource->data2 == 1) {
            pPlayer->at6b = pPlayer->at73 = 0;
        }
    }
}


void useEffectGen(XSPRITE * pXSource, spritetype * pSprite) {
    if (pSprite == NULL) pSprite = &sprite[pXSource->reference];
    if (pSprite->extra < 0) return;

    int fxId = pXSource->data2 + Random(pXSource->data3);
    int top, bottom; GetSpriteExtents(pSprite, &top, &bottom); spritetype * pEffect = NULL;
            
    if (fxId > 0 && fxId < 57 && (pEffect = gFX.fxSpawn((FX_ID) fxId, pSprite->sectnum, pSprite->x, pSprite->y, top, 0)) != NULL) {
            if ((pEffect->cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (pEffect->cstat & CSTAT_SPRITE_ONE_SIDED))
                pEffect->cstat &= ~CSTAT_SPRITE_ONE_SIDED;

        if (pSprite->flags & kModernTypeFlag1) {
            if (pEffect->pal <= 0) pEffect->pal = pSprite->pal;
            if (pEffect->xrepeat <= 0) pEffect->xrepeat = pSprite->xrepeat;
            if (pEffect->yrepeat <= 0) pEffect->yrepeat = pSprite->yrepeat;
            if (pEffect->shade == 0) pEffect->shade = pSprite->shade;
        }
    }

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSprite, pXSource->data4, -1, 0);
        // kModernTypeFlag = 2: kill the sound before play again

}


void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = NULL; bool forceWind = false; 
    int nXSector = 0;
    if (pSector == NULL) {
        
        if (sector[pSource->sectnum].extra < 0) {
            int nXSector = dbInsertXSector(pSource->sectnum);
            if (nXSector > 0) pXSector = &xsector[nXSector];
            else return;

            forceWind = true;

        } else {
            pXSector = &xsector[sector[pSource->sectnum].extra];
            nXSector = sector[pXSector->reference].extra;
        }

    } else {
        pXSector = &xsector[pSector->extra];
        nXSector = sector[pXSector->reference].extra;
    }
    
    if (pSource->flags) {
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

        short oldPan = pXSector->panVel;
        pXSector->panAngle = pXSector->windAng;
        pXSector->panVel = pXSector->windVel;

        // add to panList if panVel was set to 0 previously
        if (oldPan == 0 && pXSector->panVel != 0 && panCount < kMaxXSprites) {

            int i;
            for (i = 0; i < panCount; i++) {
                if (panList[i] != nXSector) continue;
                break;
            }

            if (i == panCount)
                panList[panCount++] = nXSector;
        }

    }
}

void useSpriteDamager(XSPRITE* pXSource, spritetype* pSprite) {
    int dmg = (pXSource->data4 == 0 || pXSource->data4 > 65534) ? 65535 : pXSource->data4;
    int dmgType = (pXSource->data3 >= 7) ? Random(6) : ((pXSource->data3 < 0) ? 0 : pXSource->data3);

    // just damage / heal TX ID sprite
    if (pSprite != NULL) {
        actDamageSprite(pSprite->xvel, pSprite, (DAMAGE_TYPE) dmgType, dmg);
        return;

        
    } // or damage / heal player# specified in data2 (or all players if data2 is empty)
    else if (pXSource->data2 > 0 && pXSource->data2 <= kMaxPlayers) {

        for (int i = connecthead; i >= 0; i = connectpoint2[i]) {
            if (pXSource->data1 < kMaxPlayers) // relative to connected players
                if (pXSource->data1 != (i + 1))
                    continue;
            else if (pXSource->data1 < (kDudePlayer1 + kMaxPlayers)) // absolute type
                if (pXSource->data1 >= kDudePlayer1 && (pXSource->data1 + (kDudePlayer1 - 1)) == gPlayer[i].pSprite->type)
                    continue;
            actDamageSprite(sprite[pXSource->reference].xvel, gPlayer[i].pSprite, (DAMAGE_TYPE) dmgType, dmg);
            return;
        }
    }
}

void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index) {
    switch (objType) {
        case 6:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(2, sector[index].extra);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqKill(1, sector[index].extra);
            }
            else {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 2, sector[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqSpawn(pXSource->data2, 1, sector[index].extra, -1);
            }
            return;

        case 0:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(0, wall[index].extra);
                if ((pXSource->data3 == 3 || pXSource->data3 == 2) && (wall[index].cstat & CSTAT_WALL_MASKED))
                    seqKill(4, wall[index].extra);
            }
            else {

                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 0, wall[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2) {

                    if (wall[index].nextwall < 0) {
                        if (pXSource->data3 == 3)
                            seqSpawn(pXSource->data2, 0, wall[index].extra, -1);

                    }
                    else {
                        if (!(wall[index].cstat & CSTAT_WALL_MASKED))
                            wall[index].cstat |= CSTAT_WALL_MASKED;

                        seqSpawn(pXSource->data2, 4, wall[index].extra, -1);
                    }
                }

                if (pXSource->data4 > 0) {

                    int cx, cy, cz, wx, wy, wz;
                    cx = (wall[index].x + wall[wall[index].point2].x) >> 1;
                    cy = (wall[index].y + wall[wall[index].point2].y) >> 1;
                    int nSector = sectorofwall(index);
                    int32_t ceilZ, floorZ;
                    getzsofslope(nSector, cx, cy, &ceilZ, &floorZ);
                    int32_t ceilZ2, floorZ2;
                    getzsofslope(wall[index].nextsector, cx, cy, &ceilZ2, &floorZ2);
                    ceilZ = ClipLow(ceilZ, ceilZ2);
                    floorZ = ClipHigh(floorZ, floorZ2);
                    wz = floorZ - ceilZ;
                    wx = wall[wall[index].point2].x - wall[index].x;
                    wy = wall[wall[index].point2].y - wall[index].y;
                    cz = (ceilZ + floorZ) >> 1;
                    
                    sfxPlay3DSound(cx, cy, cz, pXSource->data4, nSector);

                }

            }
            return;

        case 3:
            if (pXSource->data2 <= 0) seqKill(3, sprite[index].extra);
            else {
                seqSpawn(pXSource->data2, 3, sprite[index].extra, -1);
                if (pXSource->data4 > 0) sfxPlay3DSound(&sprite[index], pXSource->data4, -1, 0);
            }
            return;
    }
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

void OperateWall(int nWall, XWALL *pXWall, EVENT event) {
    walltype *pWall = &wall[nWall];
    switch (event.cmd) {
        case kCmdLock:
            pXWall->locked = 1;
            return;
        case kCmdUnlock:
            pXWall->locked = 0;
            return;
        case kCmdToggleLock:
            pXWall->locked ^= 1;
            return;
    }
    
    if (gModernMap) {
        
        // by NoOne: make 1-Way switch type for walls to work...
        switch (pWall->type) {
        case kSwitchOneWay:
            switch (event.cmd) {
                case kCmdOff:
                    SetWallState(nWall, pXWall, 0);
                    break;
                case kCmdOn:
                    SetWallState(nWall, pXWall, 1);
                    break;
                default:
                    SetWallState(nWall, pXWall, pXWall->restState ^ 1);
                    break;
                }
            return;
        }

    } else {
        
        switch (pWall->type) {
        case kWallGib:
            if (GetWallType(nWall) != pWall->type) break;
            char bStatus;
            switch (event.cmd) {
                case kCmdOn:
                case kCmdWallImpact:
                    bStatus = SetWallState(nWall, pXWall, 1);
                    break;
                case kCmdOff:
                    bStatus = SetWallState(nWall, pXWall, 0);
                    break;
                default:
                    bStatus = SetWallState(nWall, pXWall, pXWall->state ^ 1);
                    break;
            }

            if (bStatus) {
                SetupGibWallState(pWall, pXWall);
                if (pXWall->state) {
                    CGibVelocity vel(100, 100, 250);
                    int nType = ClipRange(pXWall->data, 0, 31);
                    if (nType > 0)
                        GibWall(nWall, (GIBTYPE)nType, &vel);
                }
            }
            return;
        }

    }
    
    switch (event.cmd) {
        case kCmdOff:
            SetWallState(nWall, pXWall, 0);
            break;
        case kCmdOn:
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
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
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
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
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
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
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
        // By NoOne: allow to move markers by sector movements in game if flags 1 is added in editor.
        switch (pSprite->statnum) {
            case kStatMarker:
            case kStatPathMarker:
                if (!gModernMap || !(pSprite->flags & 0x1)) continue;
                break;
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
            if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
                continue;
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (pSprite->cstat&8192)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->floorz-oldZ;
            }
            else if (pSprite->flags&2)
                pSprite->flags |= 4;
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
            if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
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
        if (sprite[nSprite].statnum == nStatus || nStatus == kStatFree)
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
        if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
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
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    spritetype *pSprite1 = &sprite[pXSector->marker0];
    spritetype *pSprite2 = &sprite[pXSector->marker1];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->type == kSectorSlide);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    spritetype *pSprite = &sprite[pXSector->marker0];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, 0, pSprite->x, pSprite->y, pSprite->ang, pSector->type == kSectorRotate);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    spritetype *pSprite = &sprite[pXSector->marker0];
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
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink);
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
    spritetype *pSprite1 = &sprite[pXSector->marker0];
    XSPRITE *pXSprite1 = &xsprite[pSprite1->extra];
    spritetype *pSprite2 = &sprite[pXSector->marker1];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nWave = pXSprite1->wave;
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, 1);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if ((a2&0xffff) == 0)
    {
        evPost(nSector, 6, (120*pXSprite2->waitTime)/10, kCmdOn);
        pXSector->state = 0;
        pXSector->busy = 0;
        if (pXSprite1->data4)
            PathSound(nSector, pXSprite1->data4);
        pXSector->marker0 = pXSector->marker1;
        pXSector->data = pXSprite2->data1;
        return 3;
    }
    return 0;
}

void OperateDoor(unsigned int nSector, XSECTOR *pXSector, EVENT event, BUSYID busyWave) 
{
    switch (event.cmd) {
        case kCmdOff:
            if (!pXSector->busy) break;
            AddBusy(nSector, busyWave, -65536/ClipLow((pXSector->busyTimeB*120)/10, 1));
            SectorStartSound(nSector, 1);
            break;
        case kCmdOn:
            if (pXSector->busy == 0x10000) break;
            AddBusy(nSector, busyWave, 65536/ClipLow((pXSector->busyTimeA*120)/10, 1));
            SectorStartSound(nSector, 0);
            break;
        default:
            if (pXSector->busy & 0xffff)  {
                if (pXSector->interruptable) {
                    ReverseBusy(nSector, busyWave);
                    pXSector->state = !pXSector->state;
                }
            } else {
                char t = !pXSector->state; int nDelta;
            
                if (t) nDelta = 65536/ClipLow((pXSector->busyTimeA*120)/10, 1);
                else nDelta = -65536/ClipLow((pXSector->busyTimeB*120)/10, 1);
            
                AddBusy(nSector, busyWave, nDelta);
                SectorStartSound(nSector, pXSector->state);
            }
            break;
    }
}

char SectorContainsDudes(int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].statnum == kStatDude)
            return 1;
    }
    return 0;
}

void TeleFrag(int nKiller, int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDude)
            actDamageSprite(nKiller, pSprite, DAMAGE_TYPE_3, 4000);
        else if (pSprite->statnum == kStatThing)
            actDamageSprite(nKiller, pSprite, DAMAGE_TYPE_3, 4000);
    }
}

void OperateTeleport(unsigned int nSector, XSECTOR *pXSector)
{
    dassert(nSector < (unsigned int)numsectors);
    int nDest = pXSector->marker0;
    dassert(nDest < kMaxSprites);
    spritetype *pDest = &sprite[nDest];
    dassert(pDest->statnum == kStatMarker);
    dassert(pDest->type == kMarkerWarpDest);
    dassert(pDest->sectnum >= 0 && pDest->sectnum < kMaxSectors);
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDude)
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

void OperatePath(unsigned int nSector, XSECTOR *pXSector, EVENT event)
{
    int nSprite;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite;
    dassert(nSector < (unsigned int)numsectors);
    spritetype *pSprite2 = &sprite[pXSector->marker0];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nId = pXSprite2->data2;
    for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == kMarkerPath)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }

    // by NoOne: trigger marker after it gets reached
    if (gModernMap && pXSprite2->state != 1)
        trTriggerSprite(pSprite2->xvel, pXSprite2, kCmdOn);

    if (nSprite < 0) {
        viewSetSystemMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
        pXSector->state = 0;
        pXSector->busy = 0;
        return;
    }
        
    pXSector->marker1 = nSprite;
    pXSector->at24_0 = pSprite2->z;
    pXSector->at28_0 = pSprite->z;
    switch (event.cmd) {
        case kCmdOn:
            pXSector->state = 0;
            pXSector->busy = 0;
            AddBusy(nSector, BUSYID_7, 65536/ClipLow((120*pXSprite2->busyTime)/10,1));
            if (pXSprite2->data3) PathSound(nSector, pXSprite2->data3);
            break;
    }
}

void OperateSector(unsigned int nSector, XSECTOR *pXSector, EVENT event)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    
    if (gModernMap) {
        switch (pSector->type) {
            // By NoOne: reset counter sector state and make it work again after unlock, so it can be used again.
            case kSectorCounter:
                switch (event.cmd) {
                    case kCmdUnlock:
                    case kCmdToggleLock:
                        if (pXSector->locked != 1) break;
                        pXSector->state = 0;
                        evPost(nSector, 6, 0, kCallbackCounterCheck);
                        break;
                }
                break;
        }
    }
    
    switch (event.cmd) {
        case kCmdLock:
            pXSector->locked = 1;
            break;
        case kCmdUnlock:
            pXSector->locked = 0;
            break;
        case kCmdToggleLock:
            pXSector->locked ^= 1;
            break;
        case kCmdStopOff:
            pXSector->stopOn = 0;
            pXSector->stopOff = 1;
            break;
        case kCmdStopOn:
            pXSector->stopOn = 1;
            pXSector->stopOff = 0;
            break;
        case kCmdStopNext:
            pXSector->stopOn = 1;
            pXSector->stopOff = 1;
            break;
        default:
            switch (pSector->type) {
                case kSectorZMotionSprite:
                    OperateDoor(nSector, pXSector, event, BUSYID_1);
                    break;
                case kSectorZMotion:
                    OperateDoor(nSector, pXSector, event, BUSYID_2);
                    break;
                case kSectorSlideMarked:
                case kSectorSlide:
                    OperateDoor(nSector, pXSector, event, BUSYID_3);
                    break;
                case kSectorRotateMarked:
                case kSectorRotate:
                    OperateDoor(nSector, pXSector, event, BUSYID_4);
                    break;
                case kSectorRotateStep:
                    switch (event.cmd) {
                        case kCmdOn:
                            pXSector->state = 0;
                            pXSector->busy = 0;
                            AddBusy(nSector, BUSYID_5, 65536/ClipLow((120*pXSector->busyTimeA)/10, 1));
                            SectorStartSound(nSector, 0);
                            break;
                        case kCmdOff:
                            pXSector->state = 1;
                            pXSector->busy = 65536;
                            AddBusy(nSector, BUSYID_5, -65536/ClipLow((120*pXSector->busyTimeB)/10, 1));
                            SectorStartSound(nSector, 1);
                            break;
                    }
                    break;
                case kSectorTeleport:
                    OperateTeleport(nSector, pXSector);
                    break;
                case kSectorPath:
                    OperatePath(nSector, pXSector, event);
                    break;
                default:
                    if (!pXSector->busyTimeA && !pXSector->busyTimeB) {
                        
                        switch (event.cmd) {
                            case kCmdOff:
                                SetSectorState(nSector, pXSector, 0);
                                break;
                            case kCmdOn:
                                SetSectorState(nSector, pXSector, 1);
                                break;
                            default:
                                SetSectorState(nSector, pXSector, pXSector->state ^ 1);
                                break;
                        }

                    } else {
                        
                        OperateDoor(nSector, pXSector, event, BUSYID_6);

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
    for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == kMarkerPath)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }
    
    if (nSprite < 0) {
        //ThrowError("Unable to find path marker with id #%d", nId);
        viewSetSystemMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
        return;
        
    }

    pXSector->marker0 = nSprite;
    basePath[nSector] = nSprite;
    if (pXSector->state)
        evPost(nSector, 6, 0, kCmdOn);
}

void LinkSector(int nSector, XSECTOR *pXSector, EVENT event)
{
    sectortype *pSector = &sector[nSector];
    int nBusy = GetSourceBusy(event);
    switch (pSector->type) {
        case kSectorZMotionSprite:
            VSpriteBusy(nSector, nBusy);
            break;
        case kSectorZMotion:
            VDoorBusy(nSector, nBusy);
            break;
        case kSectorSlideMarked:
        case kSectorSlide:
            HDoorBusy(nSector, nBusy);
            break;
        case kSectorRotateMarked:
        case kSectorRotate:
            RDoorBusy(nSector, nBusy);
            break;
         /* By NoOne: add link support for counter sectors so they can change necessary type and count of types*/
        case kSectorCounter:
            pXSector->waitTimeA = xsector[sector[event.index].extra].waitTimeA;
            pXSector->data = xsector[sector[event.index].extra].data;
            break;
        default:
            pXSector->busy = nBusy;
            if ((pXSector->busy&0xffff) == 0)
                SetSectorState(nSector, pXSector, nBusy>>16);
            break;
    }
}

void LinkSprite(int nSprite, XSPRITE *pXSprite, EVENT event) {
    spritetype *pSprite = &sprite[nSprite];
    int nBusy = GetSourceBusy(event);
    switch (pSprite->type)  {
        //By NoOne: these can be linked too now, so it's possible to change palette, underwater status and more...
        case kMarkerLowWater:
        case kMarkerUpWater:
        case kMarkerUpGoo:
        case kMarkerLowGoo:
        case kMarkerUpLink:
        case kMarkerLowLink:
        case kMarkerUpStack:
        case kMarkerLowStack: {
            if (event.type != 3) break;
            spritetype *pSprite2 = &sprite[event.index];
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
            if (event.type == 3)
            {
                int nXSprite2 = sprite[event.index].extra;
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
            if (event.type == 3)
            {
                int nSprite2 = event.index;
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

void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int command) {
    dassert(nSector < (unsigned int)numsectors);
    if (!pXSector->locked && !pXSector->isTriggered) {
        
        if (pXSector->triggerOnce) 
            pXSector->isTriggered = 1;
        
        if (pXSector->decoupled && pXSector->txID > 0)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
        
        else {
            EVENT event;
            event.cmd = command;
            OperateSector(nSector, pXSector, event);
        }

    }
}

void trMessageSector(unsigned int nSector, EVENT event) {
    dassert(nSector < (unsigned int)numsectors);
    dassert(sector[nSector].extra > 0 && sector[nSector].extra < kMaxXSectors);
    XSECTOR *pXSector = &xsector[sector[nSector].extra];
    if (!pXSector->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkSector(nSector, pXSector, event);
                break;
            case kCmdModernUse:
                pastePropertiesInObj(6, nSector, event);
                break;
            default:
                OperateSector(nSector, pXSector, event);
                break;
        }
    }
}

void trTriggerWall(unsigned int nWall, XWALL *pXWall, int command) {
    dassert(nWall < (unsigned int)numwalls);
    if (!pXWall->locked && !pXWall->isTriggered) {
        
        if (pXWall->triggerOnce)
            pXWall->isTriggered = 1;
        
        if (pXWall->decoupled && pXWall->txID > 0)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);

        else {
            EVENT event;
            event.cmd = command;
            OperateWall(nWall, pXWall, event);
        }

    }
}

void trMessageWall(unsigned int nWall, EVENT event) {
    dassert(nWall < (unsigned int)numwalls);
    dassert(wall[nWall].extra > 0 && wall[nWall].extra < kMaxXWalls);
    
    XWALL *pXWall = &xwall[wall[nWall].extra];
    if (!pXWall->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkWall(nWall, pXWall, event);
                break;
            case kCmdModernUse:
                pastePropertiesInObj(0, nWall, event);
                break;
            default:
                OperateWall(nWall, pXWall, event);
                break;
        }
    }
}

void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int command) {
    if (!pXSprite->locked && !pXSprite->isTriggered) {
        
        if (pXSprite->triggerOnce)
            pXSprite->isTriggered = 1;

        if (pXSprite->Decoupled && pXSprite->txID > 0)
           evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
        
        else {
            EVENT event;
            event.cmd = command;
            OperateSprite(nSprite, pXSprite, event);
        }

    }
}

void trMessageSprite(unsigned int nSprite, EVENT event) {
    if (sprite[nSprite].statnum != kStatFree) {

        XSPRITE* pXSprite = &xsprite[sprite[nSprite].extra];
        if (!pXSprite->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
            switch (event.cmd) {
                case kCmdLink:
                    LinkSprite(nSprite, pXSprite, event);
                    break;
                case kCmdModernUse:
                    pastePropertiesInObj(3, nSprite, event);
                    break;
                default:
                    OperateSprite(nSprite, pXSprite, event);
                    break;
            }
        }

    }
}

bool valueIsBetween(int val, int min, int max) {
    return (val > min && val < max);
}
// By NoOne: this function used by various new modern types.
void pastePropertiesInObj(int type, int nDest, EVENT event) {
   
    if (event.type != 3) return;
    spritetype* pSource = &sprite[event.index];
    
    if (pSource->extra < 0) return;
    XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (type) {
        case 6:
            if (sector[nDest].extra < 0) return;
            break;
        case 0:
            if (wall[nDest].extra < 0) return;
            break;
        case 3:
            if (sprite[nDest].extra < 0) return;
            break;
        default:
            return;
    }

    if (pSource->type == kModernConcussSprite) {
        /* - Concussing any physics affected sprite with give strength - */
        if (type != 3) return;
        else if ((sprite[nDest].flags & kPhysMove) || (sprite[nDest].flags & kPhysGravity) || isDebris(nDest))
            useConcussSprite(pXSource, &sprite[nDest]);
        return;

    } else if (pSource->type == kMarkerWarpDest) {
        /* - Allows teleport any sprite from any location to the source destination - */
        useTeleportTarget(pXSource, &sprite[nDest]);
        return;

    } else if (pSource->type == kModernSpriteDamager) {
        /* - damages xsprite via TX ID	- */
        if (type != 3) return;
        else if (xsprite[sprite[nDest].extra].health > 0) useSpriteDamager(pXSource, &sprite[nDest]);
        return;

    } if (pSource->type == kModernEffectSpawner) {
        /* - Effect Spawner can spawn any effect passed in data2 on it's or txID sprite - */
        if (pXSource->data2 < 0 || pXSource->data2 >= kFXMax) return;
        else if (type == 3)  useEffectGen(pXSource, &sprite[nDest]);
        return;

    }
    else if (pSource->type == kModernSeqSpawner) {
        /* - SEQ Spawner takes data2 as SEQ ID and spawns it on it's or TX ID sprite - */
        if (pXSource->data2 > 0 && !gSysRes.Lookup(pXSource->data2, "SEQ")) return;
        useSeqSpawnerGen(pXSource, type, nDest);
        return;
    }
    else if (pSource->type == kModernWindGenerator) {
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
    } else if (pSource->type == kModernObjDataAccumulator) {
        /* - Object Data Accumulator allows to perform sum and sub operations in data fields of object - */
        /* - data1 = destination data index 															- */
        /* - data2 = min value																			- */
        /* - data3 = max value																			- */
        /* - data4 = step value																			- */
        /* - min > max = sub, 	min < max = sum															- */

        /* - flags: 0 = force OFF if goal value was reached for all objects		     					- */
        /* - flags: 2 = force swap min and max if goal value was reached								- */
        /* - flags: 3 = force reset counter	                                           					- */

        int data = getDataFieldOfObject(type, nDest, pXSource->data1);
        if (data == -65535) return;

        if (pXSource->data2 < pXSource->data3) {

            if (data < pXSource->data2) data = pXSource->data2;
            if (data > pXSource->data3) data = pXSource->data3;

            if ((data += pXSource->data4) >= pXSource->data3) {

                switch (pSource->flags) {
                    case kModernTypeFlag0:
                    case kModernTypeFlag1:
                        if (data > pXSource->data3) data = pXSource->data3;
                        break;
                    case kModernTypeFlag2: {
                        if (data > pXSource->data3) data = pXSource->data3;
                        if (!goalValueIsReached(pXSource)) break;
                        short tmp = pXSource->data3;
                        pXSource->data3 = pXSource->data2;
                        pXSource->data2 = tmp;
                    }
                        break;
                    case kModernTypeFlag3:
                        if (data > pXSource->data3) data = pXSource->data2;
                        break;
                }
            }

        } else if (pXSource->data2 > pXSource->data3) {

            if (data > pXSource->data2) data = pXSource->data2;
            if (data < pXSource->data3) data = pXSource->data3;

            if ((data -= pXSource->data4) <= pXSource->data3) {
                switch (pSource->flags) {
                    case kModernTypeFlag0:
                    case kModernTypeFlag1:
                        if (data < pXSource->data3) data = pXSource->data3;
                        break;
                    case kModernTypeFlag2: {
                        if (data < pXSource->data3) data = pXSource->data3;
                        if (!goalValueIsReached(pXSource)) break;
                        short tmp = pXSource->data3;
                        pXSource->data3 = pXSource->data2;
                        pXSource->data2 = tmp;
                    }
                        break;
                    case kModernTypeFlag3:
                        if (data < pXSource->data3) data = pXSource->data2;
                        break;
                }
            }
        }
        
        setDataValueOfObject(type, nDest, pXSource->data1, data);
        
        return;
    
    } else if (pSource->type == kModernObjDataChanger) {

        /* - Data field changer via TX - */
        /* - data1 = sprite data1 / sector data / wall data	- */
        /* - data2 = sprite data2	- */
        /* - data3 = sprite data3	- */
        /* - data4 = sprite data4	- */

        /* - flags: 1 = treat "ignore value" as actual value - */

        switch (type) {
            case 6:
                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                    setDataValueOfObject(type, nDest, 1, pXSource->data1);
                break;

            case 3:
                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                    setDataValueOfObject(type, nDest, 1, pXSource->data1);

                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data2 != -1 && pXSource->data2 != 32767))
                    setDataValueOfObject(type, nDest, 2, pXSource->data2);

                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data3 != -1 && pXSource->data3 != 32767))
                    setDataValueOfObject(type, nDest, 3, pXSource->data3);

                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data4 != -1 && pXSource->data1 != 65535))
                    setDataValueOfObject(type, nDest, 4, pXSource->data4);
                break;

            case 0:
                if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                    setDataValueOfObject(type, nDest, 1, pXSource->data1);
                break;
        }

    } else if (pSource->type == kModernSectorFXChanger) {

        /* - FX Wave changer for sector via TX - */
            /* - data1 = Wave 	- */
            /* - data2 = Amplitude	- */
            /* - data3 = Freq	- */
            /* - data4 = Phase	- */

        if (type != 6) return;
        XSECTOR* pXSector = &xsector[sector[nDest].extra];

        if (valueIsBetween(pXSource->data1, -1, 32767))
            pXSector->wave = (pXSource->data1 > 11) ? 11 : pXSource->data1;

        int oldAmplitude = pXSector->amplitude;
        if (pXSource->data2 >= 0) pXSector->amplitude = (pXSource->data2 > 127) ? 127 : pXSource->data2;
        else if (pXSource->data2 < -1) pXSector->amplitude = (pXSource->data2 < -127) ? -127 : pXSource->data2;

        if (valueIsBetween(pXSource->data3, -1, 32767))
            pXSector->freq = (pXSource->data3 > 255) ? 255 : pXSource->data3;

        if (valueIsBetween(pXSource->data4, -1, 65535))
            pXSector->phase = (pXSource->data4 > 255) ? 255 : pXSource->data4;

        // force shadeAlways
        if (pSource->flags & kModernTypeFlag1)
            pXSector->shadeAlways = true;

        // add to shadeList if amplitude was set to 0 previously
        if (oldAmplitude == 0 && pXSector->amplitude != 0 && shadeCount < kMaxXSectors) {
            
            bool found = false;
            for (int i = 0; i < shadeCount; i++) {
                if (shadeList[i] != sector[nDest].extra) continue;
                found = true; 
                break;
            }

            if (!found)
                shadeList[shadeCount++] = sector[nDest].extra;
        }

    } else if (pSource->type == kModernDudeTargetChanger) {

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

        if (type != 3) return;
        else if (!IsDudeSprite(&sprite[nDest]) && sprite[nDest].statnum != kStatDude && xsprite[sprite[nDest].extra].data3 != 0) {
            switch (sprite[nDest].type) { // can be dead dude turned in gib
                // make current target and all other dudes not attack this dude anymore
                case kThingBloodBits:
                case kThingBloodChunks:
                    xsprite[sprite[nDest].extra].data3 = 0;
                    freeTargets(nDest);
                    return;
                default:
                    return;
            }
        }

        spritetype* pSprite = &sprite[nDest]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
        spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; int receiveHp = 33 + Random(33);
        DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase]; int matesPerEnemy = 1;

        // dude is burning?
        if (pXSprite->burnTime > 0 && pXSprite->burnSource >= 0 && pXSprite->burnSource < kMaxSprites) {
            if (IsBurningDude(pSprite)) actKillDude(pSource->xvel, pSprite, DAMAGE_TYPE_0, 65535);
            else {
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
        }

        // dude is dead?
        if (pXSprite->health <= 0) {
            pSprite->type = kThingBloodChunks; actPostSprite(pSprite->xvel, kStatThing); // turn it in gib
            return;
        }

        spritetype* pPlayer = targetIsPlayer(pXSprite);
        // special handling for player(s) if target changer data4 > 2.
        if (pPlayer != NULL) {
            if (pXSource->data4 == 3) {
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                aiSetGenIdleState(pSprite, pXSprite);
                if (pSprite->type == kDudeModernCustom)
                    removeLeech(leechIsDropped(pSprite));
            }
            else if (pXSource->data4 == 4) {
                aiSetTarget(pXSprite, pPlayer->x, pPlayer->y, pPlayer->z);
                if (pSprite->type == kDudeModernCustom)
                    removeLeech(leechIsDropped(pSprite));
            }
        }

        int maxAlarmDudes = 8 + Random(8);
        if (pXSprite->target > -1 && sprite[pXSprite->target].extra > -1 && pPlayer == NULL) {
            pTarget = &sprite[pXSprite->target]; pXTarget = &xsprite[pTarget->extra];

            if (unitCanFly(pSprite) && isMeleeUnit(pTarget) && !unitCanFly(pTarget))
                pSprite->flags |= 0x0002;
            else if (unitCanFly(pSprite))
                pSprite->flags &= ~0x0002;

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
                    DUDEINFO* pTDudeInfo = &dudeInfo[pMate->type - kDudeBase];
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
            else if (((int)gFrameClock & 256) != 0 && (pXSprite->target < 0 || getTargetDist(pSprite, pDudeInfo, pTarget) >= mDist)) {
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

        if ((pXSprite->target < 0 || pPlayer != NULL) && ((int)gFrameClock & 32) != 0) {
            // try find first target that dude can see
            for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
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
                if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || (pXSource->data2 == 1 && isMateOf(pXSprite, pXTarget)))
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
        if ((pXSprite->target < 0 || pPlayer != NULL) && pXSource->data2 == 1 && ((int)gFrameClock & 64) != 0) {
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

    } else if (pSource->type == kModernObjSizeChanger) {

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
                    sector[nDest].ceilingypanning = pXSource->data4;
                break;
            // for sprites
            case 3:
                if (valueIsBetween(pXSource->data1, -1, 32767)) {
                    if (pXSource->data1 < 1) sprite[nDest].xrepeat = 0;
                    else sprite[nDest].xrepeat = pXSource->data1;
                }

                if (valueIsBetween(pXSource->data2, -1, 32767)) {
                    if (pXSource->data2 < 1)  sprite[nDest].yrepeat = 0;
                    else sprite[nDest].yrepeat = pXSource->data2;
                }

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    sprite[nDest].xoffset = pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    sprite[nDest].yoffset = pXSource->data4;

                break;
            // for walls
            case 0:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    wall[nDest].xrepeat = pXSource->data1;

                if (valueIsBetween(pXSource->data2, -1, 32767))
                    wall[nDest].yrepeat = pXSource->data2;

                if (valueIsBetween(pXSource->data3, -1, 32767))
                    wall[nDest].xpanning = pXSource->data3;

                if (valueIsBetween(pXSource->data4, -1, 65535))
                    wall[nDest].ypanning = pXSource->data4;

                break;
        }

    } else if (pSource->type == kModernObjPicnumChanger) {

        /* - picnum changer can change picnum of sprite/wall/sector via TX ID - */
        /* - data1 = sprite pic / wall pic / sector floor pic 				 - */
        /* - data2 = sprite shade / wall overpic / sector ceil pic 		    - */
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
                    if (pSource->flags & kModernTypeFlag1)
                        pXSector->floorpal = pXSource->data3;
                }

                if (valueIsBetween(pXSource->data4, -1, 65535)) {
                    sector[nDest].ceilingpal = pXSource->data4;
                    if (pSource->flags & kModernTypeFlag1)
                        pXSector->ceilpal = pXSource->data4;
                }
                break;
            }
            // for sprites
            case 3:
                if (valueIsBetween(pXSource->data1, -1, 32767))
                    sprite[nDest].picnum = pXSource->data1;

                if (pXSource->data2 >= 0) sprite[nDest].shade = (pXSource->data2 > 127) ? 127 : pXSource->data2;
                else if (pXSource->data2 < -1) sprite[nDest].shade = (pXSource->data2 < -127) ? -127 : pXSource->data2;

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

    } else if (pSource->type == kModernObjPropertiesChanger) {
        /* - properties changer can change various properties - */
        usePropertiesChanger(pXSource, type, nDest);
            }
}

// By NoOne: the following functions required for kModernDudeTargetChanger
//---------------------------------------
spritetype* getTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode) {
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; spritetype* cTarget = NULL;
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pTarget = &sprite[nSprite];  pXTarget = &xsprite[pTarget->extra];
        if (!dudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) continue;

        int dist = getTargetDist(pSprite, pDudeInfo, pTarget);
        if (dist < minDist || dist > maxDist) continue;
        else if (pXSprite->target == pTarget->xvel) return pTarget;
        else if (!IsDudeSprite(pTarget) || pTarget->xvel == pSprite->xvel || IsPlayerSprite(pTarget)) continue;
        else if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || pTarget->owner == pSprite->xvel) continue;
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
// when kModernDudeTargetChanger goes to off state, so they won't ignore the world.
void activateDudes(int rx) {
    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type != 3) continue;
        spritetype * pDude = &sprite[rxBucket[i].index]; XSPRITE * pXDude = &xsprite[pDude->extra];
        if (!IsDudeSprite(pDude) || pXDude->aiState->stateType != kAiStateGenIdle) continue;
            aiInitSprite(pDude);
    }
}


// by NoOne: this function sets target to -1 for all dudes that hunting for nSprite
void freeTargets(int nSprite) {
    for (int nTarget = headspritestat[kStatDude]; nTarget >= 0; nTarget = nextspritestat[nTarget]) {
        if (!IsDudeSprite(&sprite[nTarget]) || sprite[nTarget].extra < 0) continue;
        else if (xsprite[sprite[nTarget].extra].target == nSprite)
            aiSetTarget(&xsprite[sprite[nTarget].extra], sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z);
    }

    return;
}

// by NoOne: this function sets target to -1 for all targets that hunting for dudes affected by selected kModernDudeTargetChanger
void freeAllTargets(XSPRITE* pXSource) {
    if (pXSource->txID <= 0) return;
    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type == 3 && sprite[rxBucket[i].index].extra >= 0)
            freeTargets(rxBucket[i].index);
    }

    return;
}

bool affectedByTargetChg(XSPRITE* pXDude) {
    if (pXDude->rxID <= 0 || pXDude->locked == 1) return false;
    for (int nSprite = headspritestat[kStatModernDudeTargetChanger]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
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
            switch (sprite[objIndex].type) {
                case kDudeModernCustom:
                    return xsprite[sprite[objIndex].extra].sysData1;
                default:
                    return xsprite[sprite[objIndex].extra].data3;
            }
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
                switch (sprite[objIndex].type) {
                    case kSwitchCombo:
                        if (xsprite[sprite[objIndex].extra].data1 == xsprite[sprite[objIndex].extra].data2) 
                            SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 1);
                        else  
                            SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 0);
                        break;
                }
                return true;
            case 2:
                xsprite[sprite[objIndex].extra].data2 = value;
                return true;
            case 3:
                switch (sprite[objIndex].type) {
                    case kDudeModernCustom:
                        xsprite[sprite[objIndex].extra].sysData1 = value;
                        break;
                    default:
                        xsprite[sprite[objIndex].extra].data3 = value;
                        break;
                }
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
        if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, pXSprite->data1) != pXSprite->data3)
            return false;
    }
    return true;
}

// by NoOne: this function tells if there any dude found for kModernDudeTargetChanger
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
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
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
    case kDudeBurningInnocent: // burning dude
    case kDudeBurningCultist: // cultist burning
    case kDudeBurningZombieAxe: // axe zombie burning
    case kDudeBurningZombieButcher: // fat zombie burning
    case kDudeBurningTinyCaleb: // tiny caleb burning
    case kDudeBurningBeast: // beast burning
    case kDudeModernCustomBurning:
        return true;
    }

    return false;
}

bool IsKillableDude(spritetype* pSprite) {
    switch (pSprite->type) {
    case kDudeGargoyleStatueFlesh: // flesh statue
    case kDudeGargoyleStatueStone: // stone statue
        return false;
    default:
        if (!IsDudeSprite(pSprite) || xsprite[pSprite->extra].locked == 1) return false;
        return true;
    }
}

bool isAnnoyingUnit(spritetype* pDude) {
    switch (pDude->type) {
    case kDudeHand: // hand
    case kDudeSpiderBrown: // brown spider
    case kDudeSpiderRed: // red spider
    case kDudeSpiderBlack: // black spider
    case kDudeSpiderMother: // mother spider
    case kDudeBoneEel: // eel
    case kDudeBat: // bat
    case kDudeRat: // rat
    case kDudePodGreen: // green pod
    case kDudeTentacleGreen: // green tentacle
    case kDudeTentacleFire: // fire tentacle
    case kDudeTentacleMother: // mother tentacle
    case kDudePodFire: // fire pod
        return true;
    default:
        return false;
    }
}

bool unitCanFly(spritetype* pDude) {
    switch (pDude->type) {
    case kDudeBat: // bat
    case kDudeGargoyleFlesh: // gargoyle
    case kDudeGargoyleStone: // stone gargoyle
    case kDudePhantasm: // phantasm
        return true;
    default:
        return false;
    }
}

bool isMeleeUnit(spritetype* pDude) {
    switch (pDude->type) {
    case kDudeZombieAxeNormal: // axe zombie
    case kDudeZombieAxeBuried: // earth zombie
    case kDudeGargoyleFlesh: // gargoyle
    case kDudeHand: // hand
    case kDudeSpiderBrown: // brown spider
    case kDudeSpiderRed: // red spider
    case kDudeSpiderBlack: // black spider
    case kDudeSpiderMother: // mother spider
    case kDudeGillBeast: // gill beast
    case kDudeBoneEel: // eel
    case kDudeBat: // bat
    case kDudeRat: // rat
    case kDudeTentacleGreen: // green tentacle
    case kDudeTentacleFire: // fire tentacle
    case kDudeTentacleMother: // mother tentacle
    case kDudeZombieAxeLaying: // sleep zombie
    case kDudeInnocent: // innocent
    case kDudeTinyCaleb: // tiny caleb
    case kDudeBeast: // beast
        return true;
    case kDudeModernCustom:
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
                    if (pSprite->flags&2)
                        pSprite->flags |= 4;
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
            sprite[i].inittype = sprite[i].type;
            baseSprite[i].x = sprite[i].x;
            baseSprite[i].y = sprite[i].y;
            baseSprite[i].z = sprite[i].z;
        }
        else
            sprite[i].inittype = -1;
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
            switch (pSector->type)
            {
            case kSectorCounter:
                //By NoOne: no need to trigger once it, instead lock so it can be unlocked and used again.
                if (!gModernMap) pXSector->triggerOnce = 1;
                evPost(i, 6, 0, kCallbackCounterCheck);
                break;
            case kSectorZMotion:
            case kSectorZMotionSprite:
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            case kSectorSlideMarked:
            case kSectorSlide:
            {
                spritetype *pSprite1 = &sprite[pXSector->marker0];
                spritetype *pSprite2 = &sprite[pXSector->marker1];
                TranslateSector(i, 0, -65536, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->type == kSectorSlide);
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
                TranslateSector(i, 0, pXSector->busy, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->type == kSectorSlide);
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            }
            case kSectorRotateMarked:
            case kSectorRotate:
            {
                spritetype *pSprite1 = &sprite[pXSector->marker0];
                TranslateSector(i, 0, -65536, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, 0, pSprite1->x, pSprite1->y, pSprite1->ang, pSector->type == kSectorRotate);
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
                TranslateSector(i, 0, pXSector->busy, pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, 0, pSprite1->x, pSprite1->y, pSprite1->ang, pSector->type == kSectorRotate);
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            }
            case kSectorPath:
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
            switch (sprite[i].type) {
            case kSwitchPadlock:
                pXSprite->triggerOnce = 1;
                break;
            case kModernRandom:
            case kModernRandom2:
            case kModernSeqSpawner:
            case kModernObjDataAccumulator:
            case kModernDudeTargetChanger:
            case kModernEffectSpawner:
            case kModernWindGenerator:
            case kGenTrigger:
            case kGenDripWater:
            case kGenDripBlood:
            case kGenMissileFireball:
            case kGenModernMissileUniversal:
            case kGenDart:
            case kGenBubble:
            case kGenBubbleMulti:
            case kGenSound:
                InitGenerator(i);
                break;
            case kThingArmedProxBomb:
            case kModernThingTNTProx:
                pXSprite->Proximity = 1;
                break;
            case kThingFallingRock:
                if (pXSprite->state) sprite[i].flags |= 7;
                else sprite[i].flags &= ~7;
                break;
            }
            if (pXSprite->Vector)
                sprite[i].cstat |= 256;
            if (pXSprite->Push)
                sprite[i].cstat |= 4096;
        }
    }
    evSend(0, 0, 7, kCmdOn);
    if (gGameOptions.nGameType == 1)
        evSend(0, 0, 9, kCmdOn);
    else if (gGameOptions.nGameType == 2)
        evSend(0, 0, 8, kCmdOn);
    else if (gGameOptions.nGameType == 3)
    {
        evSend(0, 0, 8, kCmdOn);
        evSend(0, 0, 10, kCmdOn);
    }
}

void trTextOver(int nId)
{
    char *pzMessage = levelGetMessage(nId);
    if (pzMessage)
        viewSetMessage(pzMessage, VanillaMode() ? 0 : 8, MESSAGE_PRIORITY_INI); // 8: gold
}

void InitGenerator(int nSprite)
{
    dassert(nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->statnum != kMaxStatus);
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    switch (sprite[nSprite].type) {
        // By NoOne: intialize modern generators
        case kModernRandom:
        case kModernRandom2:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            if (pXSprite->state != pXSprite->restState)
                evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
            return;
        case kModernDudeTargetChanger:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            if (pXSprite->busyTime <= 0) pXSprite->busyTime = 5;
            if (pXSprite->state != pXSprite->restState)
                evPost(nSprite, 3, 0, kCmdRepeat);
            return;
        case kModernEffectSpawner:
        case kModernSeqSpawner:
            if (pXSprite->state != pXSprite->restState)
                evPost(nSprite, 3, 0, kCmdRepeat);
            return;
        case kModernObjDataAccumulator:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            if (pXSprite->state != pXSprite->restState)
                evPost(nSprite, 3, 0, kCmdRepeat);
            return;
        case kModernWindGenerator:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            if (pXSprite->state != pXSprite->restState)
                evPost(nSprite, 3, 0, kCmdRepeat);
            return;
        case kGenTrigger:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            break;
    }
    if (pXSprite->state != pXSprite->restState && pXSprite->busyTime > 0)
        evPost(nSprite, 3, (120*(pXSprite->busyTime+Random2(pXSprite->data1)))/10, kCmdRepeat);
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
        case kModernRandom:
        case kModernRandom2: {
            // let's first search for previously dropped items and remove it
            if (pXSprite->dropMsg > 0) {
                for (short nItem = headspritestat[kStatItem]; nItem >= 0; nItem = nextspritestat[nItem]) {
                    spritetype* pItem = &sprite[nItem];
                    if (pItem->type == pXSprite->dropMsg && pItem->x == pSprite->x && pItem->y == pSprite->y && pItem->z == pSprite->z) {
                        gFX.fxSpawn((FX_ID)29, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
                        deletesprite(nItem);
                        break;
                    }
                }
            }

            // then drop item
            spritetype* pDrop = DropRandomPickupObject(pSprite, pXSprite->dropMsg);

            // check if generator affected by physics
            if (pDrop != NULL && isDebris(pSprite->xvel) != -1 && (pDrop->extra >= 0 || dbInsertXSprite(pDrop->xvel) > 0)) {
                int nIndex = debrisGetFreeIndex();
                if (nIndex >= 0) {
                    xsprite[pDrop->extra].physAttr |= kPhysMove | kPhysGravity | kPhysFalling; // must fall always
                    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;

                    gPhysSpritesList[nIndex] = pDrop->xvel; 
                    if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
                    getSpriteMassBySize(pDrop); // create mass cache
                }
            }
            break;
        }
        case kGenDripWater:
        case kGenDripBlood: {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            actSpawnThing(pSprite->sectnum, pSprite->x, pSprite->y, bottom, (pSprite->type == kGenDripWater) ? kThingDripWater : kThingDripBlood);
            break;
        }
        case kGenSound: {
            // By NoOne: allow custom pitch and volume for sounds in SFX gen.
            if (!gModernMap) sfxPlay3DSound(pSprite, pXSprite->data2, -1, 0);
            else {
                int pitch = pXSprite->data4 << 1; if (pitch < 2000) pitch = 0;
                sfxPlay3DSoundCP(pSprite, pXSprite->data2, -1, 0, pitch, pXSprite->data3);
            }
            break;
        }
        case kGenMissileFireball:
            switch (pXSprite->data2) {
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
        case kGenModernMissileUniversal:
            if (gModernMap) UniMissileTrapSeqCallback(3, nXSprite);
            break;
        case kGenBubble:
        case kGenBubbleMulti: {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            gFX.fxSpawn((pSprite->type == kGenBubble) ? FX_23 : FX_26, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
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
        actFireMissile(pSprite, 0, 0, 0, 0, (pSprite->cstat&8) ? 0x4000 : -0x4000, kMissileFireball);
    else
        actFireMissile(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, kMissileFireball);
}

// By NoOne: Callback for trap that can fire any missile specified in data1
void UniMissileTrapSeqCallback(int, int nXSprite)
{
    
    XSPRITE* pXSprite = &xsprite[nXSprite]; int dx = 0, dy = 0, dz = 0;
    spritetype* pSprite = &sprite[pXSprite->reference];
    
    if (pXSprite->data1 < kMissileBase || pXSprite->data1 >= kMissileMax) 
        return;

    if (pSprite->cstat & 32) {
        if (pSprite->cstat & 8) dz = 0x4000;
        else dz = -0x4000;
    } else {
        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = pXSprite->data3 << 6; // add slope controlling
        if (dz > 0x10000) dz = 0x10000;
        else if (dz < -0x10000) dz = -0x10000;
    }

    spritetype* pMissile = NULL;
    pMissile = actFireMissile(pSprite, 0, 0, dx, dy, dz, pXSprite->data1);
    if (pMissile != NULL) {

        // inherit some properties of the generator
        if (pSprite->flags & kModernTypeFlag1) {
            
            pMissile->xrepeat = pSprite->xrepeat;
            pMissile->yrepeat = pSprite->yrepeat;

            pMissile->pal = pSprite->pal;
            pMissile->shade = pSprite->shade;

        }

        // add velocity controlling
        if (pXSprite->data2 > 0) {
            
            int velocity = pXSprite->data2 << 12;
            xvel[pMissile->xvel] = mulscale(velocity, dx, 14);
            yvel[pMissile->xvel] = mulscale(velocity, dy, 14);
            zvel[pMissile->xvel] = mulscale(velocity, dz, 14);

        }

        // add bursting for missiles
        if (pMissile->type != kMissileFlareAlt && pXSprite->data4 > 0)
            evPost(pMissile->xvel, 3, (pXSprite->data4 > 500) ? 500 : pXSprite->data4 - 1, kCallbackMissileBurst);

    }

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
                evPost(nSprite, 3, 1, kCmdOff);
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

END_BLD_NS
