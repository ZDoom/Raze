//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

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


///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////
#include "ns.h"

#ifdef NOONE_EXTENSIONS
#include <random>
#include "blood.h"
#include "savegamehelp.h"

BEGIN_BLD_NS

inline int mulscale8(int a, int b) { return MulScale(a, b, 8); }

bool gAllowTrueRandom = false;
bool gEventRedirectsUsed = false;
SPRITEMASS gSpriteMass[];   // cache for getSpriteMassBySize();
short gProxySpritesList[];  // list of additional sprites which can be triggered by Proximity
short gProxySpritesCount;   // current count
short gSightSpritesList[];  // list of additional sprites which can be triggered by Sight
short gSightSpritesCount;   // current count
short gPhysSpritesList[];   // list of additional sprites which can be affected by physics
short gPhysSpritesCount;    // current count
short gImpactSpritesList[];
short gImpactSpritesCount;


TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];

TRCONDITION gCondition[kMaxTrackingConditions];
short gTrackingCondsCount;

std::default_random_engine gStdRandom;

VECTORINFO_EXTRA gVectorInfoExtra[] = {
    1207,1207,      1001,1001,      4001,4002,
    431,431,        1002,1002,      359,359,
    521,521,        513,513,        499,499,
    9012,9014,      1101,1101,      1207,1207,
    499,495,        495,496,        9013,499,
    1307,1308,      499,499,        499,499,
    499,499,        499,499,        351,351,
    0,0,            357,499
};

MISSILEINFO_EXTRA gMissileInfoExtra[] = {
    1207, 1207,    false, false, false, false, false, true, false,     true,
    420, 420,      false, true, true, false, false, false, false,      true,
    471, 471,      false, false, false, false, false, false, true,     false,
    421, 421,      false, true, false, true, false, false, false,      false,
    1309, 351,     false, true, false, false, false, false, false,     true,
    480, 480,      false, true, false, true, false, false, false,      false,
    470, 470,      false, false, false, false, false, false, true,     true,
    489, 490,      false, false, false, false, false, true, false,     true,
    462, 351,      false, true, false, false, false, false, false,     true,
    1203, 172,     false, false, true, false, false, false, false,     true,
    0,0,           false, false, true, false, false, false, false,     true,
    1457, 249,     false, false, false, false, false, true, false,     true,
    480, 489,      false, true, false, true, false, false, false,      false,
    480, 489,      false, false, false, true, false, false, false,     false,
    480, 489,      false, false, false, true, false, false, false,     false,
    491, 491,      true, true, true, true, true, true, true,           true,
    520, 520,      false, false, false, false, false, true, false,     true,
    520, 520,      false, false, false, false, false, true, false,     true,
};

THINGINFO_EXTRA gThingInfoExtra[] = {
    true,   true,   true,   false,  false,
    false,  false,  false,  false,  false,
    false,  false,  false,  false,  false,
    true,   false,  false,  true,   true,
    true,   true,   false,  false,  false,
    false,  false,  true,   true,   true,
    true,   true,   true,   true,   true,
    true,
};

DUDEINFO_EXTRA gDudeInfoExtra[] = {
    
    { false,  false,  1, -1, -1, -1, -1, -1 },      // 200
    { false,  false,  0, 9, 13, 13, 17, 14 },       // 201
    { false,  false,  0, 9, 13, 13, 17, 14 },       // 202
    { false,  true,   0, 8, 0, 8, -1, -1 },         // 203
    { false,  false,  0, 8, 0, 8, -1, -1 },         // 204
    { false,  true,   1, -1, -1, -1, -1, -1 },      // 205
    { true,   true,   0, 0, 0, 0, -1, -1 },         // 206
    { true,   false,  0, 0, 0, 0, -1, -1 },         // 207
    { true,   false,  1, -1, -1, -1, -1, -1 },      // 208
    { true,   false,  1, -1, -1, -1, -1, -1 },      // 209
    { true,   true,   0, 0, 0, 0, -1, -1 },         // 210
    { false,  true,   0, 8, 0, 8, -1, -1 },         // 211
    { false,  true,   0, 6, 0, 6, -1, -1 },         // 212
    { false,  true,   0, 7, 0, 7, -1, -1 },         // 213
    { false,  true,   0, 7, 0, 7, -1, -1 },         // 214
    { false,  true,   0, 7, 0, 7, -1, -1 },         // 215
    { false,  true,   0, 7, 0, 7, -1, -1 },         // 216
    { false,  true,   0, 9, 10, 10, -1, -1 },       // 217
    { false,  true,   0, 0, 0, 0, -1, -1 },         // 218
    { true,  false,  7, 7, 7, 7, -1, -1 },          // 219
    { false,  true,   0, 7, 0, 7, -1, -1 },         // 220
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 221
    { false,  true,   -1, -1, -1, -1, -1, -1 },     // 222
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 223
    { false,  true,   -1, -1, -1, -1, -1, -1 },     // 224
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 225
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 226
    { false,  false,  0, 7, 0, 7, -1, -1 },         // 227
    { false,  false,  0, 7, 0, 7, -1, -1 },         // 228
    { false,  false,  0, 8, 0, 8, -1, -1 },         // 229
    { false,  false,  0, 9, 13, 13, 17, 14 },       // 230
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 231
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 232
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 233
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 234
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 235
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 236
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 237
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 238
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 239
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 240
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 241
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 242
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 243
    { false,  true,   -1, -1, -1, -1, -1, -1 },     // 244
    { false,  true,   0, 6, 0, 6, -1, -1 },         // 245
    { false,  false,  0, 9, 13, 13, 17, 14 },       // 246
    { false,  false,  0, 9, 13, 13, 14, 14 },       // 247
    { false,  false,  0, 9, 13, 13, 14, 14 },       // 248
    { false,  false,  0, 9, 13, 13, 17, 14 },       // 249
    { false,  true,   0, 6, 8, 8, 10, 9 },          // 250
    { false,  true,   0, 8, 9, 9, 11, 10 },         // 251
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 252
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 253
    { false,  false,  0, 9, 17, 13, 17, 14 },       // 254
    { false,  false,  -1, -1, -1, -1, -1, -1 },     // 255

};


AISTATE genPatrolStates[] = {

    //-------------------------------------------------------------------------------

    { kAiStatePatrolWaitL, 0, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitL, 7, -1, 0, NULL, NULL, aiPatrolThink, NULL },

    { kAiStatePatrolMoveL, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveL, 8, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveL, 0, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveL, 6, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveL, 7, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

    //-------------------------------------------------------------------------------

    { kAiStatePatrolWaitW, 0, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitW, 10, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitW, 13, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitW, 17, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitW, 8, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitW, 9, -1, 0, NULL, NULL, aiPatrolThink, NULL },

    { kAiStatePatrolMoveW, 0, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 10, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 13, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 8, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 7, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveW, 6, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

    //-------------------------------------------------------------------------------

    { kAiStatePatrolWaitC, 17, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitC, 11, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitC, 10, -1, 0, NULL, NULL, aiPatrolThink, NULL },
    { kAiStatePatrolWaitC, 14, -1, 0, NULL, NULL, aiPatrolThink, NULL },

    { kAiStatePatrolMoveC, 14, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveC, 10, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
    { kAiStatePatrolMoveC, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

    //-------------------------------------------------------------------------------

};


// for actor.cpp
//-------------------------------------------------------------------------

bool nnExtIsImmune(spritetype* pSprite, int dmgType, int minScale) {

    if (dmgType >= kDmgFall && dmgType < kDmgMax && pSprite->extra >= 0 && xsprite[pSprite->extra].locked != 1) {
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            return (thingInfo[pSprite->type - kThingBase].dmgControl[dmgType] <= minScale);
        else if (IsDudeSprite(pSprite)) {
            if (IsPlayerSprite(pSprite)) return (gPlayer[pSprite->type - kDudePlayer1].damageControl[dmgType]);
            else if (pSprite->type == kDudeModernCustom) return (gGenDudeExtra[pSprite->index].dmgControl[dmgType] <= minScale);
            else return (getDudeInfo(pSprite->type)->damageVal[dmgType] <= minScale);
        }
    }

    return true;
}

bool nnExtEraseModernStuff(spritetype* pSprite, XSPRITE* pXSprite) {
    
    bool erased = false;
    switch (pSprite->type) {
        // erase all modern types if the map is not extended
        case kModernCustomDudeSpawn:
        case kModernRandomTX:
        case kModernSequentialTX:
        case kModernSeqSpawner:
        case kModernObjPropertiesChanger:
        case kModernObjPicnumChanger:
        case kModernObjSizeChanger:
        case kModernDudeTargetChanger:
        case kModernSectorFXChanger:
        case kModernObjDataChanger:
        case kModernSpriteDamager:
        case kModernObjDataAccumulator:
        case kModernEffectSpawner:
        case kModernWindGenerator:
        case kModernPlayerControl:
        case kModernCondition:
        case kModernConditionFalse:
            pSprite->type = kSpriteDecoration;
            erased = true;
            break;
        case kItemModernMapLevel:
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
        case kModernThingTNTProx:
        case kModernThingEnemyLifeLeech:
            pSprite->type = kSpriteDecoration;
            changespritestat(pSprite->index, kStatDecoration);
            erased = true;
            break;
        // also erase some modernized vanilla types which was not active
        case kMarkerWarpDest:
            if (pSprite->statnum == kStatMarker) break;
            pSprite->type = kSpriteDecoration;
            erased = true;
            break;
    }

    if (pXSprite->Sight) {
        pXSprite->Sight = false; // it does not work in vanilla at all
        erased = true;
    }

    if (pXSprite->Proximity) {
        // proximity works only for things and dudes in vanilla
        switch (pSprite->statnum) {
            case kStatThing:
            case kStatDude:
                break;
            default:
                pXSprite->Proximity = false;
                erased = true;
                break;
        }
    }

    return erased;
}

void nnExtTriggerObject(int objType, int objIndex, int command) {
    switch (objType) {
        case OBJ_SECTOR:
            if (!xsectRangeIsFine(sector[objIndex].extra)) break;
            trTriggerSector(objIndex, &xsector[sector[objIndex].extra], command);
            break;
        case OBJ_WALL:
            if (!xwallRangeIsFine(wall[objIndex].extra)) break;
            trTriggerWall(objIndex, &xwall[wall[objIndex].extra], command);
            break;
        case OBJ_SPRITE:
            if (!xspriRangeIsFine(sprite[objIndex].extra)) break;
            trTriggerSprite(objIndex, &xsprite[sprite[objIndex].extra], command);
            break;
    }

    return;
}

void nnExtResetGlobals() {
    gAllowTrueRandom = gEventRedirectsUsed = false;

    // reset counters
    gProxySpritesCount = gSightSpritesCount = gPhysSpritesCount = gImpactSpritesCount = 0;

    // fill arrays with negative values to avoid index 0 situation
    memset(gSightSpritesList, -1, sizeof(gSightSpritesList));   memset(gProxySpritesList, -1, sizeof(gProxySpritesList));
    memset(gPhysSpritesList, -1, sizeof(gPhysSpritesList));     memset(gImpactSpritesList, -1, sizeof(gImpactSpritesList));

    // reset tracking conditions, if any
    if (gTrackingCondsCount > 0) {
        for (int i = 0; i < gTrackingCondsCount; i++) {
            TRCONDITION* pCond = &gCondition[i];
            for (unsigned k = 0; k < pCond->length; k++) {
                pCond->obj[k].index = pCond->obj[k].cmd = 0;
                pCond->obj[k].type = -1;
            }

            pCond->length = 0;
        }

        gTrackingCondsCount = 0;
    }
}

void nnExtInitModernStuff(bool bSaveLoad) {
    
    nnExtResetGlobals();

    // use true random only for single player mode, otherwise use Blood's default one.
    if (gGameOptions.nGameType == 0 && !VanillaMode() && !DemoRecordStatus()) {
        
        gStdRandom.seed(std::random_device()());

        // since true random is not working if compiled with old mingw versions, we should
        // check if it works in game and if not - switch to using in-game random function.
        for (int i = kMaxRandomizeRetries; i >= 0; i--) {
            std::uniform_int_distribution<int> dist_a_b(0, 100);
            if (gAllowTrueRandom || i <= 0) break;
            else if (dist_a_b(gStdRandom) != 0)
                gAllowTrueRandom = true;
        }

    }

    
    for (int i = 0; i < kMaxXSprites; i++) {

        if (xsprite[i].reference < 0) continue;
        XSPRITE* pXSprite = &xsprite[i];  spritetype* pSprite = &sprite[pXSprite->reference];
        
        switch (pSprite->type) {
            case kModernRandomTX:
            case kModernSequentialTX:
                if (pXSprite->command == kCmdLink) gEventRedirectsUsed = true;
                break;
            case kDudeModernCustom:
            case kDudeModernCustomBurning:
                getSpriteMassBySize(pSprite); // create mass cache
                break;
            case kModernCondition:
            case kModernConditionFalse:
                if (bSaveLoad) break;
                else if (!pXSprite->rxID) condError(pXSprite,"\nThe condition must have RX ID!\nSPRITE #%d", pSprite->index);
                else if (!pXSprite->txID && !pSprite->flags) {
                    Printf(PRINT_HIGH, "The condition must have TX ID or hitag to be set: RX ID %d, SPRITE #%d", pXSprite->rxID, pSprite->index);
                }
                break;
        }

        // init after loading save file
        if (bSaveLoad) {

            // add in list of physics affected sprites
            if (pXSprite->physAttr != 0) {
                //xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;

                gPhysSpritesList[gPhysSpritesCount++] = pSprite->index; // add sprite index
                getSpriteMassBySize(pSprite); // create mass cache
            }

            if (pXSprite->data3 != pXSprite->sysData1) {
                switch (pSprite->statnum) {
                case kStatDude:
                    switch (pSprite->type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        pXSprite->data3 = pXSprite->sysData1; // move sndStartId back from sysData1 to data3 
                        break;
                    }
                    break;
                }
            }

        } else {
            
            // auto set going On and going Off if both are empty
            if (pXSprite->txID && !pXSprite->triggerOn && !pXSprite->triggerOff)
                pXSprite->triggerOn = pXSprite->triggerOff = true;
            
            // copy custom start health to avoid overwrite by kThingBloodChunks
            if (IsDudeSprite(pSprite))
                pXSprite->sysData2 = pXSprite->data4;
            
            // check reserved statnums
            if (pSprite->statnum >= kStatModernBase && pSprite->statnum < kStatModernMax) {
                bool sysStat = true;
                switch (pSprite->statnum) {
                    case kStatModernDudeTargetChanger:
                        sysStat = (pSprite->type != kModernDudeTargetChanger);
                        break;
                    case kStatModernCondition:
                        sysStat = (pSprite->type != kModernCondition && pSprite->type != kModernConditionFalse);
                        break;
                    case kStatModernEventRedirector:
                        sysStat = (pSprite->type != kModernRandomTX && pSprite->type != kModernSequentialTX);
                        break;
                    case kStatModernPlayerLinker:
                    case kStatModernQavScene:
                        sysStat = (pSprite->type != kModernPlayerControl);
                        break;
                }

                if (sysStat)
                    I_Error("Sprite status list number %d on sprite #%d is in a range of reserved (%d - %d)!", pSprite->index, pSprite->statnum, kStatModernBase, kStatModernMax);
            }

            switch (pSprite->type) {
                case kModernRandomTX:
                case kModernSequentialTX:
                    if (pXSprite->command != kCmdLink) break;
                    // add statnum for faster redirects search
                    changespritestat(pSprite->index, kStatModernEventRedirector);
                    break;
                case kModernWindGenerator:
                    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                    break;
                case kModernDudeTargetChanger:
                case kModernObjDataAccumulator:
                case kModernRandom:
                case kModernRandom2:
                    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                    pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                    switch (pSprite->type) {
                        // add statnum for faster dude searching
                        case kModernDudeTargetChanger:
                            changespritestat(pSprite->index, kStatModernDudeTargetChanger);
                            if (pXSprite->busyTime <= 0) pXSprite->busyTime = 5;
                            pXSprite->command = kCmdLink;
                            break;
                        // remove kStatItem status from random item generators
                        case kModernRandom:
                        case kModernRandom2:
                            changespritestat(pSprite->index, kStatDecoration);
                            break;
                    }
                    break;
                case kModernThingTNTProx:
                    pXSprite->Proximity = true;
                    break;
                case kDudeModernCustom: 
                {
                    if (pXSprite->txID <= 0) break;
                    int nSprite, found = 0;
                    StatIterator it(kStatDude);
                    while ((nSprite = it.NextIndex()) >= 0)
                    {
                        XSPRITE* pXSpr = &xsprite[sprite[nSprite].extra];
                        if (pXSpr->rxID != pXSprite->txID) continue;
                        else if (found) I_Error("\nCustom dude (TX ID %d):\nOnly one incarnation allowed per channel!", pXSprite->txID);
                        changespritestat(nSprite, kStatInactive);
                        found++;
                    }
                    break;
                }
                case kDudePodMother:
                case kDudeTentacleMother:
                    pXSprite->state = 1;
                    break;
                case kModernPlayerControl:
                    switch (pXSprite->command) {
                        case kCmdLink:
                        {
                            if (pXSprite->data1 < 1 || pXSprite->data1 >= kMaxPlayers)
                                I_Error("\nPlayer Control (SPRITE #%d):\nPlayer out of a range (data1 = %d)", pSprite->index, pXSprite->data1);

                            //if (numplayers < pXSprite->data1)
                                //I_Error("\nPlayer Control (SPRITE #%d):\n There is no player #%d", pSprite->index, pXSprite->data1);

                            if (pXSprite->rxID && pXSprite->rxID != kChannelLevelStart)
                                I_Error("\nPlayer Control (SPRITE #%d) with Link command should have no RX ID!", pSprite->index);

                            if (pXSprite->txID && pXSprite->txID < kChannelUser)
                                I_Error("\nPlayer Control (SPRITE #%d):\nTX ID should be in range of %d and %d!", pSprite->index, kChannelUser, kChannelMax);

                            // only one linker per player allowed
                            int nSprite;
                            StatIterator it(kStatModernPlayerLinker);
                            while ((nSprite = it.NextIndex()) >= 0)
                            {
                                XSPRITE* pXCtrl = &xsprite[sprite[nSprite].extra];
                                if (pXSprite->data1 == pXCtrl->data1)
                                    I_Error("\nPlayer Control (SPRITE #%d):\nPlayer %d already linked with different player control sprite #%d!", pSprite->index, pXSprite->data1, nSprite);
                            }
                            pXSprite->sysData1 = -1;
                            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                            changespritestat(pSprite->index, kStatModernPlayerLinker);
                            break;
                        }
                        case 67: // play qav animation
                            if (pXSprite->txID && !pXSprite->waitTime) pXSprite->waitTime = 1;
                            changespritestat(pSprite->index, kStatModernQavScene);
                            break;
                    }
                    break;
                case kModernCondition:
                case kModernConditionFalse:
                    if (pXSprite->busyTime > 0) {
                        
                        if (pXSprite->waitTime > 0) {
                            pXSprite->busyTime += ClipHigh(((pXSprite->waitTime * 120) / 10), 4095); pXSprite->waitTime = 0;
                            Printf(PRINT_HIGH, "Summing busyTime and waitTime for tracking condition #%d, RX ID %d. Result = %d ticks", pSprite->index, pXSprite->rxID, pXSprite->busyTime);
                        }

                        pXSprite->busy = pXSprite->busyTime;
                    }
                    
                    if (pXSprite->waitTime && pXSprite->command >= kCmdNumberic)
                        condError(pXSprite, "Delay is not available when using numberic commands (%d - %d)", kCmdNumberic, 255);

                    pXSprite->Decoupled = false; // must go through operateSprite always
                    pXSprite->Sight     = pXSprite->Impact  = pXSprite->Touch   = pXSprite->triggerOff     = false;
                    pXSprite->Proximity = pXSprite->Push    = pXSprite->Vector  = pXSprite->triggerOn      = false;
                    pXSprite->state = pXSprite->restState = 0;
                    
                    pXSprite->targetX = pXSprite->targetY = pXSprite->targetZ = pXSprite->target = pXSprite->sysData2 = -1;
                    changespritestat(pSprite->index, kStatModernCondition);
                    int oldStat = pSprite->cstat; pSprite->cstat = 0x30;
                    
                    if (oldStat & CSTAT_SPRITE_BLOCK) 
                        pSprite->cstat |= CSTAT_SPRITE_BLOCK;
                    
                    if (oldStat & 0x2000) pSprite->cstat |= 0x2000;
                    else if (oldStat & 0x4000) pSprite->cstat |= 0x4000;

                    pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                    break;
            }

            // the following trigger flags are senseless to have together
            if ((pXSprite->Touch && (pXSprite->Proximity || pXSprite->Sight) && pXSprite->DudeLockout)
                    || (pXSprite->Touch && pXSprite->Proximity && !pXSprite->Sight)) pXSprite->Touch = false;

            if (pXSprite->Proximity && pXSprite->Sight && pXSprite->DudeLockout)
                pXSprite->Proximity = false;
            
            // very quick fix for floor sprites with Touch trigger flag if their Z is equals sector floorz / ceilgz
            if (pSprite->sectnum >= 0 && pXSprite->Touch && (pSprite->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) {
                if (pSprite->z == sector[pSprite->sectnum].floorz) pSprite->z--;
                else if (pSprite->z == sector[pSprite->sectnum].ceilingz) pSprite->z++;
            }
        }

        // make Proximity flag work not just for dudes and things...
        if (pXSprite->Proximity && gProxySpritesCount < kMaxSuperXSprites) {
            switch (pSprite->statnum) {
                case kStatFX:           case kStatExplosion:            case kStatItem:
                case kStatPurge:        case kStatSpares:               case kStatFlare:
                case kStatInactive:     case kStatFree:                 case kStatMarker:
                case kStatPathMarker:   case kStatThing:                case kStatDude:
                case kStatModernPlayerLinker:
                    break;
                default:
                    gProxySpritesList[gProxySpritesCount++] = pSprite->index;
                    if (gProxySpritesCount == kMaxSuperXSprites)
                        I_Error("Max (%d) *additional* Proximity sprites reached!", kMaxSuperXSprites);
                    break;
            }
        }

        // make Sight flag work not just for dudes and things...
        if (pXSprite->Sight && gSightSpritesCount < kMaxSuperXSprites) {
            switch (pSprite->statnum) {
                case kStatFX:           case kStatExplosion:            case kStatItem:
                case kStatPurge:        case kStatSpares:               case kStatFlare:
                case kStatInactive:     case kStatFree:                 case kStatMarker:
                case kStatPathMarker:   case kStatModernPlayerLinker:
                    break;
                default:
                    gSightSpritesList[gSightSpritesCount++] = pSprite->index;
                    if (gSightSpritesCount == kMaxSuperXSprites)
                        I_Error("Max (%d) Sight sprites reached!", kMaxSuperXSprites);
                    break;
            }
        }

        // make Impact flag work for sprites that affected by explosions...
        if (pXSprite->Impact && gImpactSpritesCount < kMaxSuperXSprites) {
            switch (pSprite->statnum) {
                case kStatFX:           case kStatExplosion:            case kStatItem:
                case kStatPurge:        case kStatSpares:               case kStatFlare:
                case kStatInactive:     case kStatFree:                 case kStatMarker:
                case kStatPathMarker:   case kStatModernPlayerLinker:
                    break;
                default:
                    gImpactSpritesList[gImpactSpritesCount++] = pSprite->index;
                    if (gImpactSpritesCount == kMaxSuperXSprites)
                        I_Error("Max (%d) *additional* Impact sprites reached!", kMaxSuperXSprites);
                    break;
            }
        }
    }

    int i;
    if (!bSaveLoad) {

        // let's try to find "else" and "else if" of conditions here
        spritetype* pCond = NULL; XSPRITE* pXCond = NULL;
        bool found = false; int rx = 0; int sum1 = 0; int sum2 = 0;
        
        for (i = StatIterator::First(kStatModernCondition); i >= 0;) {
            pCond = &sprite[i]; pXCond = &xsprite[pCond->extra];
            sum1 = pXCond->locked + pXCond->busyTime + pXCond->waitTime + pXCond->data1;
            if (!found) rx = pXCond->rxID;
            
            for (int a = i; a >= 0; a = StatIterator::NextFor(a), found = false) {
                spritetype* pCond2 = &sprite[a]; XSPRITE* pXCond2 = &xsprite[pCond2->extra];
                sum2 = pXCond2->locked + pXCond2->busyTime + pXCond2->waitTime + pXCond2->data1;

                if (pXCond2->rxID != rx || pCond2->index == pCond->index || sum1 != sum2) continue;
                else if ((pCond2->type != pCond->type) ^ (pCond2->cstat != pCond->cstat)) {
                    Printf("> ELSE IF found for condition #%d (RX ID: %d, CONDID: %d)\n", i, rx, pXCond->data1);
                    pXCond2->rxID = pXCond2->busyTime = 0;
                    pXCond->sysData2 = pCond2->index;
                    i = a; found = true;
                    break;
                }

            }

            if (!found) i = StatIterator::NextFor(i);
        }

    }

    // collect objects for tracking conditions
    StatIterator it(kStatModernCondition);
    while ((i = it.NextIndex()) >= 0)
    {
        spritetype* pSprite = &sprite[i]; XSPRITE* pXSprite = &xsprite[pSprite->extra];

        if (pXSprite->busyTime <= 0 || pXSprite->isTriggered) continue;
        else if (gTrackingCondsCount >= kMaxTrackingConditions)
            I_Error("\nMax (%d) tracking conditions reached!", kMaxTrackingConditions);
            
        int count = 0;
        TRCONDITION* pCond = &gCondition[gTrackingCondsCount];

        for (int i = 0; i < kMaxXSprites; i++) {
            if (!spriRangeIsFine(xsprite[i].reference) || xsprite[i].txID != pXSprite->rxID || xsprite[i].reference == pSprite->index)
                continue;

            XSPRITE* pXSpr = &xsprite[i]; spritetype* pSpr = &sprite[pXSpr->reference];
            int index = pXSpr->reference; int cmd = pXSpr->command;
            switch (pSpr->type) {
                case kSwitchToggle: // exceptions
                case kSwitchOneWay: // exceptions
                    continue;
                case kModernPlayerControl:
                    if (pSpr->statnum != kStatModernPlayerLinker || !bSaveLoad) break;
                    // assign player sprite after savegame loading
                    index = pXSpr->sysData1;
                    cmd = xsprite[sprite[index].extra].command;
                    break;
            }

            if (pSpr->type == kModernCondition || pSpr->type == kModernConditionFalse)
                condError(pXSprite, "Tracking condition always must be first in condition sequence!");

            if (count >= kMaxTracedObjects)
                condError(pXSprite, "Max(% d) objects to track reached for condition # % d, rx id : % d!");

            pCond->obj[count].type = OBJ_SPRITE;
            pCond->obj[count].index = index;
            pCond->obj[count++].cmd = cmd;
        }

        for (int i = 0; i < kMaxXSectors; i++) {
            if (!sectRangeIsFine(xsector[i].reference) || xsector[i].txID != pXSprite->rxID) continue;
            else if (count >= kMaxTracedObjects)
                condError(pXSprite, "Max(% d) objects to track reached for condition # % d, rx id : % d!");

            pCond->obj[count].type = OBJ_SECTOR;
            pCond->obj[count].index = xsector[i].reference;
            pCond->obj[count++].cmd = xsector[i].command;
        }

        for (int i = 0; i < kMaxXWalls; i++) {
            if (!wallRangeIsFine(xwall[i].reference) || xwall[i].txID != pXSprite->rxID)
                continue;

            walltype* pWall = &wall[xwall[i].reference];
            switch (pWall->type) {
                case kSwitchToggle: // exceptions
                case kSwitchOneWay: // exceptions
                    continue;
            }

            if (count >= kMaxTracedObjects)
                condError(pXSprite, "Max(% d) objects to track reached for condition # % d, rx id : % d!");
                
            pCond->obj[count].type = OBJ_WALL;
            pCond->obj[count].index = xwall[i].reference;
            pCond->obj[count++].cmd = xwall[i].command;
        }

        if (count == 0)
            Printf(PRINT_HIGH, "No objects to track found for condition #%d, rx id: %d!", pSprite->index, pXSprite->rxID);

        pCond->length = count;
        pCond->xindex = pSprite->extra;
        gTrackingCondsCount++;

    }
}


// The following functions required for random event features
//-------------------------
int nnExtRandom(int a, int b) {
    if (!gAllowTrueRandom) return Random(((b + 1) - a)) + a;
    // used for better randomness in single player
    std::uniform_int_distribution<int> dist_a_b(a, b);
    return dist_a_b(gStdRandom);
}

int GetDataVal(spritetype* pSprite, int data) {
    assert(xspriRangeIsFine(pSprite->extra));
    
    switch (data) {
        case 0: return xsprite[pSprite->extra].data1;
        case 1: return xsprite[pSprite->extra].data2;
        case 2: return xsprite[pSprite->extra].data3;
        case 3: return xsprite[pSprite->extra].data4;
    }

    return -1;
}

// tries to get random data field of sprite
int randomGetDataValue(XSPRITE* pXSprite, int randType) {
    if (pXSprite == NULL) return -1;
    int random = 0; int bad = 0; int maxRetries = kMaxRandomizeRetries;

    int rData[4];
    rData[0] = pXSprite->data1; rData[2] = pXSprite->data3;
    rData[1] = pXSprite->data2; rData[3] = pXSprite->data4;
    // randomize only in case if at least 2 data fields fits.
    for (int i = 0; i < 4; i++) {
        switch (randType) {
        case kRandomizeItem:
            if (rData[i] >= kItemWeaponBase && rData[i] < kItemMax) break;
            else bad++;
            break;
        case kRandomizeDude:
            if (rData[i] >= kDudeBase && rData[i] < kDudeMax) break;
            else bad++;
            break;
        case kRandomizeTX:
            if (rData[i] > kChannelZero && rData[i] < kChannelUserMax) break;
            else bad++;
            break;
        default:
            bad++;
            break;
        }
    }

    if (bad < 3) {
        // try randomize few times
        while (maxRetries > 0) {
            random = nnExtRandom(0, 3);
            if (rData[random] > 0) return rData[random];
            else maxRetries--;
        }
    }

    return -1;
}

// this function drops random item using random pickup generator(s)
spritetype* randomDropPickupObject(spritetype* pSource, short prevItem) {
    spritetype* pSprite2 = NULL; int selected = -1; int maxRetries = 9;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        while ((selected = randomGetDataValue(pXSource, kRandomizeItem)) == prevItem) if (maxRetries-- <= 0) break;
        if (selected > 0) {
            pSprite2 = actDropObject(pSource, selected);
            if (pSprite2 != NULL) {

                pXSource->dropMsg = uint8_t(pSprite2->type); // store dropped item type in dropMsg
                pSprite2->x = pSource->x;
                pSprite2->y = pSource->y;
                pSprite2->z = pSource->z;

                if ((pSource->flags & kModernTypeFlag1) && (pXSource->txID > 0 || (pXSource->txID != 3 && pXSource->lockMsg > 0)) &&
                    dbInsertXSprite(pSprite2->index) > 0) {

                    XSPRITE* pXSprite2 = &xsprite[pSprite2->extra];

                    // inherit spawn sprite trigger settings, so designer can send command when item picked up.
                    pXSprite2->txID = pXSource->txID;
                    pXSprite2->command = pXSource->command;
                    pXSprite2->triggerOn = pXSource->triggerOn;
                    pXSprite2->triggerOff = pXSource->triggerOff;

                    pXSprite2->Pickup = true;

                }
            }
        }
    }
    return pSprite2;
}

// this function spawns random dude using dudeSpawn
spritetype* randomSpawnDude(spritetype* pSource) {
    spritetype* pSprite2 = NULL; int selected = -1;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if ((selected = randomGetDataValue(pXSource, kRandomizeDude)) > 0)
            pSprite2 = actSpawnDude(pSource, selected, -1, 0);
    }
    return pSprite2;
}
//-------------------------

void nnExtProcessSuperSprites() {

    // process tracking conditions
    if (gTrackingCondsCount > 0) {
        for (int i = 0; i < gTrackingCondsCount; i++) {

            TRCONDITION* pCond = &gCondition[i]; XSPRITE* pXCond = &xsprite[pCond->xindex];
            if (pCond->length > 0 && !pXCond->locked && !pXCond->isTriggered && ++pXCond->busy >= pXCond->busyTime) {

                pXCond->busy = 0;
                for (unsigned k = 0; k < pCond->length; k++) {

                    EVENT evn;
                    evn.index = pCond->obj[k].index;   evn.cmd = pCond->obj[k].cmd;
                    evn.type = pCond->obj[k].type;     evn.funcID = kCallbackMax;
                    useCondition(&sprite[pXCond->reference], pXCond, evn);

                }

            }
        }
    }
    
    // process additional proximity sprites
    if (gProxySpritesCount > 0) {
        for (int i = 0; i < gProxySpritesCount; i++) {
            if (sprite[gProxySpritesList[i]].extra < 0) continue;

            XSPRITE* pXProxSpr = &xsprite[sprite[gProxySpritesList[i]].extra];
            if (!pXProxSpr->Proximity || (!pXProxSpr->Interrutable && pXProxSpr->state != pXProxSpr->restState) || pXProxSpr->locked == 1
                || pXProxSpr->isTriggered) continue;  // don't process locked or triggered sprites

            int x = sprite[gProxySpritesList[i]].x;	int y = sprite[gProxySpritesList[i]].y;
            int z = sprite[gProxySpritesList[i]].z;	int index = sprite[gProxySpritesList[i]].index;
            int sectnum = sprite[gProxySpritesList[i]].sectnum;

            if (!pXProxSpr->DudeLockout) {

                int nAffected;
                StatIterator it(kStatDude);
                while ((nAffected = it.NextIndex()) >= 0)
                {
                    if ((sprite[nAffected].flags & 32) || xsprite[sprite[nAffected].extra].health <= 0) continue;
                    else if (CheckProximity(&sprite[nAffected], x, y, z, sectnum, 96)) {
                        trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity);
                        break;
                    }
                }

            } else {

                for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                    if (gPlayer[a].pXSprite->health > 0 && CheckProximity(gPlayer[a].pSprite, x, y, z, sectnum, 96)) {
                        trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity);
                        break;
                    }
                }

            }
        }
    }

    // process sight sprites (for players only)
    if (gSightSpritesCount > 0) {
        for (int i = 0; i < gSightSpritesCount; i++) {
            if (sprite[gSightSpritesList[i]].extra < 0) continue;

            XSPRITE* pXSightSpr = &xsprite[sprite[gSightSpritesList[i]].extra];
            if (!pXSightSpr->Sight || (!pXSightSpr->Interrutable && pXSightSpr->state != pXSightSpr->restState) || pXSightSpr->locked == 1 ||
                pXSightSpr->isTriggered) continue; // don't process locked or triggered sprites

            int x = sprite[gSightSpritesList[i]].x;	int y = sprite[gSightSpritesList[i]].y;
            int z = sprite[gSightSpritesList[i]].z;	int index = sprite[gSightSpritesList[i]].index;
            int sectnum = sprite[gSightSpritesList[i]].sectnum;

            for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                spritetype* pPlaySprite = gPlayer[a].pSprite;
                if (gPlayer[a].pXSprite->health > 0 && cansee(x, y, z, sectnum, pPlaySprite->x, pPlaySprite->y, pPlaySprite->z, pPlaySprite->sectnum)) {
                    trTriggerSprite(index, pXSightSpr, kCmdSpriteSight);
                    break;
                }
            }
        }
    }

    // process Debris sprites for movement
    if (gPhysSpritesCount > 0) {
        for (int i = 0; i < gPhysSpritesCount; i++) {
            if (gPhysSpritesList[i] == -1) continue;
            else if (sprite[gPhysSpritesList[i]].statnum == kStatFree || (sprite[gPhysSpritesList[i]].flags & kHitagFree) != 0) {
                gPhysSpritesList[i] = -1;
                continue;
            }

            XSPRITE* pXDebris = &xsprite[sprite[gPhysSpritesList[i]].extra];
            if (!(pXDebris->physAttr & kPhysMove) && !(pXDebris->physAttr & kPhysGravity)) {
                gPhysSpritesList[i] = -1;
                continue;
            }

            spritetype* pDebris = &sprite[gPhysSpritesList[i]];
            int idx = pDebris->index;

            XSECTOR* pXSector = (sector[pDebris->sectnum].extra >= 0) ? &xsector[sector[pDebris->sectnum].extra] : NULL;
            viewBackupSpriteLoc(idx, pDebris);
            
            bool uwater = false;
            int mass = gSpriteMass[pDebris->extra].mass;
            int airVel = gSpriteMass[pDebris->extra].airVel;

                    int top, bottom;
                    GetSpriteExtents(pDebris, &top, &bottom);
            
            if (pXSector != NULL) {
                
                if ((uwater = pXSector->Underwater) != 0) airVel <<= 6;
                if (pXSector->panVel != 0 && getflorzofslope(pDebris->sectnum, pDebris->x, pDebris->y) <= bottom) {
                    
                    int angle = pXSector->panAngle; int speed = 0;
                    if (pXSector->panAlways || pXSector->state || pXSector->busy) {
                            speed = pXSector->panVel << 9;
                            if (!pXSector->panAlways && pXSector->busy)
                                speed = MulScale(speed, pXSector->busy, 16);
                        }
                        if (sector[pDebris->sectnum].floorstat & 64)
                            angle = (angle + GetWallAngle(sector[pDebris->sectnum].wallptr) + 512) & 2047;
                        int dx = MulScale(speed, Cos(angle), 30);
                        int dy = MulScale(speed, Sin(angle), 30);
                    xvel[idx] += dx;
                    yvel[idx] += dy;

                    }
                
                }

            actAirDrag(pDebris, airVel);

            if (pXDebris->physAttr & kPhysDebrisTouch) {
                PLAYER* pPlayer = NULL;
                for (int a = connecthead; a != -1; a = connectpoint2[a]) {
                    pPlayer = &gPlayer[a];
                    if ((gSpriteHit[pPlayer->pSprite->extra].hit & 0xc000) == 0xc000  && (gSpriteHit[pPlayer->pSprite->extra].hit & 0x3fff) == idx) {
                        
                            int nSpeed = approxDist(xvel[pPlayer->pSprite->index], yvel[pPlayer->pSprite->index]);
                            nSpeed = ClipLow(nSpeed - MulScale(nSpeed, mass, 6), 0x9000 - (mass << 3));

                            xvel[idx] += MulScale(nSpeed, Cos(pPlayer->pSprite->ang), 30);
                            yvel[idx] += MulScale(nSpeed, Sin(pPlayer->pSprite->ang), 30);
                            
                            gSpriteHit[pDebris->extra].hit = pPlayer->pSprite->index | 0xc000;

                    }
                }
            }
            
            if (pXDebris->physAttr & kPhysGravity) pXDebris->physAttr |= kPhysFalling;
            if ((pXDebris->physAttr & kPhysFalling) || xvel[idx] || yvel[idx] || zvel[idx] || velFloor[pDebris->sectnum] || velCeil[pDebris->sectnum])
            debrisMove(i);

            if (xvel[idx] || yvel[idx])
                pXDebris->goalAng = getangle(xvel[idx], yvel[idx]) & 2047;

            int ang = pDebris->ang & 2047;
            if ((uwater = spriteIsUnderwater(pDebris)) == false) evKill(idx, 3, kCallbackEnemeyBubble);
            else if (Chance(0x1000 - mass)) {
                
                if (zvel[idx] > 0x100) debrisBubble(idx);
                if (ang == pXDebris->goalAng) {
                   pXDebris->goalAng = (pDebris->ang + Random3(kAng60)) & 2047;
                   debrisBubble(idx);
        }

    }

            int angStep = ClipLow(mulscale8(1, ((abs(xvel[idx]) + abs(yvel[idx])) >> 5)), (uwater) ? 1 : 0);
            if (ang < pXDebris->goalAng) pDebris->ang = ClipHigh(ang + angStep, pXDebris->goalAng);
            else if (ang > pXDebris->goalAng) pDebris->ang = ClipLow(ang - angStep, pXDebris->goalAng);

            int nSector = pDebris->sectnum;
            int cz = getceilzofslope(nSector, pDebris->x, pDebris->y);
            int fz = getflorzofslope(nSector, pDebris->x, pDebris->y);
            
            GetSpriteExtents(pDebris, &top, &bottom);
            if (fz >= bottom && gLowerLink[nSector] < 0 && !(sector[nSector].ceilingstat & 0x1)) pDebris->z += ClipLow(cz - top, 0);
            if (cz <= top && gUpperLink[nSector] < 0 && !(sector[nSector].floorstat & 0x1)) pDebris->z += ClipHigh(fz - bottom, 0);

        }
    }

}

// this function plays sound predefined in missile info
void sfxPlayMissileSound(spritetype* pSprite, int missileId) {
    MISSILEINFO_EXTRA* pMissType = &gMissileInfoExtra[missileId - kMissileBase];
    sfxPlay3DSound(pSprite, Chance(0x5000) ? pMissType->fireSound[0] : pMissType->fireSound[1], -1, 0);
}

// this function plays sound predefined in vector info
void sfxPlayVectorSound(spritetype* pSprite, int vectorId) {
    VECTORINFO_EXTRA* pVectorData = &gVectorInfoExtra[vectorId];
    sfxPlay3DSound(pSprite, Chance(0x5000) ? pVectorData->fireSound[0] : pVectorData->fireSound[1], -1, 0);
}

int getSpriteMassBySize(spritetype* pSprite) {
    int mass = 0; int seqId = -1; int clipDist = pSprite->clipdist; Seq* pSeq = NULL;
    if (pSprite->extra < 0) {
        I_Error("getSpriteMassBySize: pSprite->extra < 0");

    } else if (IsDudeSprite(pSprite)) {

        switch (pSprite->type) {
        case kDudePodMother: // fake dude, no seq
            break;
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
            seqId = xsprite[pSprite->extra].data2;
            clipDist = gGenDudeExtra[pSprite->index].initVals[2];
            break;
        default:
            seqId = getDudeInfo(pSprite->type)->seqStartID;
            break;
        }

    } else  {

        seqId = seqGetID(3, pSprite->extra);

    }

    SPRITEMASS* cached = &gSpriteMass[pSprite->extra];
    if (((seqId >= 0 && seqId == cached->seqId) || pSprite->picnum == cached->picnum) && pSprite->xrepeat == cached->xrepeat &&
        pSprite->yrepeat == cached->yrepeat && clipDist == cached->clipdist) {
        return cached->mass;
    }

    short picnum = pSprite->picnum;
    short massDiv = 30;  short addMul = 2; short subMul = 2;

    if (seqId >= 0) {
        auto pSeq = getSequence(seqId);
        if (pSeq)
        {
            picnum = seqGetTile(&pSeq->frames[0]);
        } else
            picnum = pSprite->picnum;
    }

    clipDist = ClipLow(pSprite->clipdist, 1);
    short x = tileWidth(picnum);        short y = tileHeight(picnum);
    short xrepeat = pSprite->xrepeat; 	short yrepeat = pSprite->yrepeat;

    // take surface type into account
    switch (tileGetSurfType(pSprite->index + 0xc000)) {
        case 1:  massDiv = 16; break; // stone
        case 2:  massDiv = 18; break; // metal
        case 3:  massDiv = 21; break; // wood
        case 4:  massDiv = 25; break; // flesh
        case 5:  massDiv = 28; break; // water
        case 6:  massDiv = 26; break; // dirt
        case 7:  massDiv = 27; break; // clay
        case 8:  massDiv = 35; break; // snow
        case 9:  massDiv = 22; break; // ice
        case 10: massDiv = 37; break; // leaves
        case 11: massDiv = 33; break; // cloth
        case 12: massDiv = 36; break; // plant
        case 13: massDiv = 24; break; // goo
        case 14: massDiv = 23; break; // lava
    }

    mass = ((x + y) * (clipDist / 2)) / massDiv;

    if (xrepeat > 64) mass += ((xrepeat - 64) * addMul);
    else if (xrepeat < 64 && mass > 0) {
        for (int i = 64 - xrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (yrepeat > 64) mass += ((yrepeat - 64) * addMul);
    else if (yrepeat < 64 && mass > 0) {
        for (int i = 64 - yrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (mass <= 0) cached->mass = 1 + Random(10);
    else cached->mass = ClipRange(mass, 1, 65535);

    cached->airVel = ClipRange(400 - cached->mass, 32, 400);
    cached->fraction = ClipRange(60000 - (cached->mass << 7), 8192, 60000);

    cached->xrepeat = pSprite->xrepeat;             cached->yrepeat = pSprite->yrepeat;
    cached->picnum = pSprite->picnum;               cached->seqId = seqId;
    cached->clipdist = pSprite->clipdist;

    return cached->mass;
}

int debrisGetIndex(int nSprite) {
    if (sprite[nSprite].extra < 0 || xsprite[sprite[nSprite].extra].physAttr == 0)
        return -1;

    for (int i = 0; i < gPhysSpritesCount; i++) {
        if (gPhysSpritesList[i] != nSprite) continue;
        return i;
    }

    return -1;
}

int debrisGetFreeIndex(void) {
    for (int i = 0; i < kMaxSuperXSprites; i++) {
        if (gPhysSpritesList[i] == -1 || sprite[gPhysSpritesList[i]].statnum == kStatFree) return i;

        else if ((sprite[gPhysSpritesList[i]].flags & kHitagFree) || sprite[gPhysSpritesList[i]].extra < 0) return i;
        else if (xsprite[sprite[gPhysSpritesList[i]].extra].physAttr == 0) return i;
    }

    return -1;
}

void debrisConcuss(int nOwner, int listIndex, int x, int y, int z, int dmg) {
    spritetype* pSprite = (gPhysSpritesList[listIndex] >= 0) ? &sprite[gPhysSpritesList[listIndex]] : NULL;
    if (pSprite != NULL && xspriRangeIsFine(pSprite->extra)) {
        int dx = pSprite->x - x; int dy = pSprite->y - y; int dz = (pSprite->z - z) >> 4;
        dmg = scale(0x40000, dmg, 0x40000 + dx * dx + dy * dy + dz * dz);

        int size = (tileWidth(pSprite->picnum) * pSprite->xrepeat * tileHeight(pSprite->picnum) * pSprite->yrepeat) >> 1;
        if (xsprite[pSprite->extra].physAttr & kPhysDebrisExplode) {
            if (gSpriteMass[pSprite->extra].mass > 0) {
                int t = scale(dmg, size, gSpriteMass[pSprite->extra].mass);

                xvel[pSprite->index] += MulScale(t, dx, 16);
                yvel[pSprite->index] += MulScale(t, dy, 16);
                zvel[pSprite->index] += MulScale(t, dz, 16);
            }

            if (pSprite->type >= kThingBase && pSprite->type < kThingMax && !xsprite[pSprite->extra].locked)
                changespritestat(pSprite->index, kStatThing); // if it was a thing, return it's statnum back
        }


        actDamageSprite(nOwner, pSprite, DAMAGE_TYPE_3, dmg);
        return;
    }
}

void debrisBubble(int nSprite) {
    
    spritetype* pSprite = &sprite[nSprite];
    
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (int i = 0; i < 1 + Random(5); i++) {
        
        int nDist = (pSprite->xrepeat * (tileWidth(pSprite->picnum) >> 1)) >> 2;
        int nAngle = Random(2048);
        int x = pSprite->x + MulScale(nDist, Cos(nAngle), 30);
        int y = pSprite->y + MulScale(nDist, Sin(nAngle), 30);
        int z = bottom - Random(bottom - top);
        spritetype* pFX = gFX.fxSpawn((FX_ID)(FX_23 + Random(3)), pSprite->sectnum, x, y, z, 0);
        if (pFX) {
            xvel[pFX->index] = xvel[nSprite] + Random2(0x1aaaa);
            yvel[pFX->index] = yvel[nSprite] + Random2(0x1aaaa);
            zvel[pFX->index] = zvel[nSprite] + Random2(0x1aaaa);
        }

    }
    
    if (Chance(0x2000))
        evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
}

void debrisMove(int listIndex) {

    if (!(sprite[gPhysSpritesList[listIndex]].extra > 0 && sprite[gPhysSpritesList[listIndex]].extra < kMaxXSprites)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    } else if (!(sprite[gPhysSpritesList[listIndex]].sectnum >= 0 && sprite[gPhysSpritesList[listIndex]].sectnum < kMaxSectors)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    }

    int nSprite = gPhysSpritesList[listIndex];
    int nXSprite = sprite[nSprite].extra;       XSPRITE* pXDebris = &xsprite[nXSprite];
    spritetype* pSprite = &sprite[nSprite];     int nSector = pSprite->sectnum;

    int top, bottom, i;
    GetSpriteExtents(pSprite, &top, &bottom);

    int moveHit = 0;
    int floorDist = (bottom - pSprite->z) >> 2;
    int ceilDist = (pSprite->z - top) >> 2;
    int clipDist = pSprite->clipdist << 2;
    int mass = gSpriteMass[nXSprite].mass;

    bool uwater = false, depth = false;
    int tmpFraction = gSpriteMass[pSprite->extra].fraction;
    if (sector[nSector].extra >= 0 && xsector[sector[nSector].extra].Underwater) {
        tmpFraction >>= 1;
        uwater = true;
    }

    if (xvel[nSprite] || yvel[nSprite]) {

        short oldcstat = pSprite->cstat;
        pSprite->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        moveHit = gSpriteHit[nXSprite].hit = ClipMove((int*)&pSprite->x, (int*)&pSprite->y, (int*)&pSprite->z, &nSector, xvel[nSprite] >> 12,
            yvel[nSprite] >> 12, clipDist, ceilDist, floorDist, CLIPMASK0);

        pSprite->cstat = oldcstat;
        if (pSprite->sectnum != nSector) {
            if (!sectRangeIsFine(nSector)) return;
            else ChangeSpriteSect(nSprite, nSector);
        }

        if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate) {
            short nSector2 = nSector;
            if (pushmove_old(&pSprite->x, &pSprite->y, &pSprite->z, &nSector2, clipDist, ceilDist, floorDist, CLIPMASK0) != -1)
                nSector = nSector2;
        }

        if ((gSpriteHit[nXSprite].hit & 0xc000) == 0x8000) {
            i = moveHit = gSpriteHit[nXSprite].hit & 0x3fff;
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], i, tmpFraction);
        }

    } else if (!FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector)) {
        return;
    }

        if (pSprite->sectnum != nSector) {
            assert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        nSector = pSprite->sectnum;
        }

    if (sector[nSector].extra > 0) {
        uwater = xsector[sector[nSector].extra].Underwater;
        depth = xsector[sector[nSector].extra].Depth;
        }

    if (zvel[nSprite])
        pSprite->z += zvel[nSprite] >> 8;

    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, clipDist, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
    GetSpriteExtents(pSprite, &top, &bottom);

    if ((pXDebris->physAttr & kPhysDebrisSwim) && uwater) {

        int vc = 0;
        int cz = getceilzofslope(nSector, pSprite->x, pSprite->y);
        int fz = getflorzofslope(nSector, pSprite->x, pSprite->y);
        int div = ClipLow(bottom - top, 1);

        if (gLowerLink[nSector] >= 0) cz += (cz < 0) ? 0x500 : -0x500;
        if (top > cz && (!(pXDebris->physAttr & kPhysDebrisFloat) || fz <= bottom << 2))
            zvel[nSprite] -= DivScale((bottom - ceilZ) >> 6, mass, 8);

        if (fz < bottom)
            vc = 58254 + ((bottom - fz) * -80099) / div;

        if (vc) {
            pSprite->z += ((vc << 2) >> 1) >> 8;
            zvel[nSprite] += vc;
        }

    } else if ((pXDebris->physAttr & kPhysGravity) && bottom < floorZ) {

        pSprite->z += 455;
        zvel[nSprite] += 58254;

    }

    if ((i = CheckLink(pSprite)) != 0) {
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, clipDist, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
        if (!(pSprite->cstat & CSTAT_SPRITE_INVISIBLE)) {
            switch (i) {
            case kMarkerUpWater:
            case kMarkerUpGoo:
                int pitch = (150000 - (gSpriteMass[pSprite->extra].mass << 9)) + Random3(8192);
                sfxPlay3DSoundCP(pSprite, 720, -1, 0, pitch, 75 - Random(40));
                    if (!spriteIsUnderwater(pSprite)) {
                    evKill(pSprite->index, 3, kCallbackEnemeyBubble);
                    } else {
                        evPost(pSprite->index, 3, 0, kCallbackEnemeyBubble);
                    for (int i = 2; i <= 5; i++) {
                            if (Chance(0x5000 * i))
                                evPost(pSprite->index, 3, Random(5), kCallbackEnemeyBubble);
                    }
                }
                break;
            }
        }
    }

    GetSpriteExtents(pSprite, &top, &bottom);

    if (floorZ <= bottom) {

        gSpriteHit[nXSprite].florhit = floorHit;
        int v30 = zvel[nSprite] - velFloor[pSprite->sectnum];

        if (v30 > 0) {

            pXDebris->physAttr |= kPhysFalling;
            actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v30, pSprite->sectnum, tmpFraction);
            zvel[nSprite] = v30;

            if (abs(zvel[nSprite]) < 0x10000) {
                zvel[nSprite] = velFloor[pSprite->sectnum];
                pXDebris->physAttr &= ~kPhysFalling;
            }

            moveHit = floorHit;
            spritetype* pFX = NULL; spritetype* pFX2 = NULL;
            switch (tileGetSurfType(floorHit)) {
            case kSurfLava:
                if ((pFX = gFX.fxSpawn(FX_10, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0)) == NULL) break;
                for (i = 0; i < 7; i++) {
                    if ((pFX2 = gFX.fxSpawn(FX_14, pFX->sectnum, pFX->x, pFX->y, pFX->z, 0)) == NULL) continue;
                    xvel[pFX2->index] = Random2(0x6aaaa);
                    yvel[pFX2->index] = Random2(0x6aaaa);
                    zvel[pFX2->index] = -(int)Random(0xd5555);
                }
                break;
            case kSurfWater:
                gFX.fxSpawn(FX_9, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0);
                break;
            }

        } else if (zvel[nSprite] == 0) {

            pXDebris->physAttr &= ~kPhysFalling;

        }

    } else {

        gSpriteHit[nXSprite].florhit = 0;
        if (pXDebris->physAttr & kPhysGravity)
            pXDebris->physAttr |= kPhysFalling;

    }

    if (top <= ceilZ) {

        gSpriteHit[nXSprite].ceilhit = moveHit = ceilHit;
        pSprite->z += ClipLow(ceilZ - top, 0);
        if (zvel[nSprite] <= 0 && (pXDebris->physAttr & kPhysFalling))
            zvel[nSprite] = MulScale(-zvel[nSprite], 0x2000, 16);

    } else {

        gSpriteHit[nXSprite].ceilhit = 0;
        GetSpriteExtents(pSprite, &top, &bottom);

    }

    if (moveHit && pXDebris->Impact && !pXDebris->locked && !pXDebris->isTriggered && (pXDebris->state == pXDebris->restState || pXDebris->Interrutable)) {
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            changespritestat(nSprite, kStatThing);

        trTriggerSprite(pSprite->index, pXDebris, kCmdToggle);

    }

    if (!xvel[nSprite] && !yvel[nSprite]) return;
    else if ((floorHit & 0xc000) == 0xc000) {

            int nHitSprite = floorHit & 0x3fff;
        if ((sprite[nHitSprite].cstat & 0x30) == 0) {
                xvel[nSprite] += MulScale(4, pSprite->x - sprite[nHitSprite].x, 2);
                yvel[nSprite] += MulScale(4, pSprite->y - sprite[nHitSprite].y, 2);
            return;
            }
        }

    pXDebris->height = ClipLow(floorZ - bottom, 0) >> 8;
    if (uwater || pXDebris->height >= 0x100)
        return;

    int nDrag = 0x2a00;
    if (pXDebris->height > 0)
        nDrag -= scale(nDrag, pXDebris->height, 0x100);

    xvel[nSprite] -= mulscale16r(xvel[nSprite], nDrag);
    yvel[nSprite] -= mulscale16r(yvel[nSprite], nDrag);
    if (approxDist(xvel[nSprite], yvel[nSprite]) < 0x1000)
        xvel[nSprite] = yvel[nSprite] = 0;

}



bool ceilIsTooLow(spritetype* pSprite) {
    if (pSprite != NULL) {

        sectortype* pSector = &sector[pSprite->sectnum];
        int a = pSector->ceilingz - pSector->floorz;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int b = top - bottom;
        if (a > b) return true;
    }

    return false;
}

void aiSetGenIdleState(spritetype* pSprite, XSPRITE* pXSprite) 
{
    auto actor = &bloodActors[pXSprite->reference];
    switch (pSprite->type) {
    case kDudeModernCustom:
    case kDudeModernCustomBurning:
        aiGenDudeNewState(pSprite, &genIdle);
        break;
    default:
        aiNewState(actor, &genIdle);
        break;
    }
}

// this function stops wind on all TX sectors affected by WindGen after it goes off state.
void windGenStopWindOnSectors(XSPRITE* pXSource) {
    spritetype* pSource = &sprite[pXSource->reference];
    if (pXSource->txID <= 0 && xsectRangeIsFine(sector[pSource->sectnum].extra)) {
        xsector[sector[pSource->sectnum].extra].windVel = 0;
        return;
    }

    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type != OBJ_SECTOR) continue;
        XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
        if ((pXSector->state == 1 && !pXSector->windAlways)
            || ((pSource->flags & kModernTypeFlag1) && !(pSource->flags & kModernTypeFlag2))) {
                pXSector->windVel = 0;
        }
    }
    
    // check redirected TX buckets
    int rx = -1; XSPRITE* pXRedir = NULL;
    while ((pXRedir = evrListRedirectors(OBJ_SPRITE, sprite[pXSource->reference].extra, pXRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            if (rxBucket[i].type != OBJ_SECTOR) continue;
            XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
            if ((pXSector->state == 1 && !pXSector->windAlways) || (pSource->flags & kModernTypeFlag2))
                pXSector->windVel = 0;
        }
    }
}


void trPlayerCtrlStartScene(XSPRITE* pXSource, PLAYER* pPlayer, bool force) {

    int nSource = pXSource->reference; TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];

    if (pCtrl->qavScene.index >= 0 && !force) return;
    
    QAV* pQav = playerQavSceneLoad(pXSource->data2);
    if (pQav != NULL) {

        // save current weapon
        pXSource->dropMsg = pPlayer->curWeapon;

        short nIndex = pCtrl->qavScene.index;
        if (nIndex > -1 && nIndex != nSource && sprite[nIndex].extra >= 0)
            pXSource->dropMsg = xsprite[sprite[nIndex].extra].dropMsg;

        if (nIndex < 0)
            WeaponLower(pPlayer);

        pXSource->sysData1 = ClipLow((pQav->duration * pXSource->waitTime) / 4, 0); // how many times animation should be played

        pCtrl->qavScene.index = nSource;
        pCtrl->qavScene.qavResrc = pQav;
        pCtrl->qavScene.dummy = -1;

        //pCtrl->qavScene.qavResrc->Preload();

        pPlayer->sceneQav = pXSource->data2;
        pPlayer->weaponTimer = pCtrl->qavScene.qavResrc->duration;
        pPlayer->qavCallback = (pXSource->data3 > 0) ? ClipRange(pXSource->data3 - 1, 0, 32) : -1;
        pPlayer->qavLoop = false;

    }

}

void trPlayerCtrlStopScene(PLAYER* pPlayer) {

    TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];
    int scnIndex = pCtrl->qavScene.index; XSPRITE* pXSource = NULL;
    if (spriRangeIsFine(scnIndex)) {
        pXSource = &xsprite[sprite[scnIndex].extra];
        pXSource->sysData1 = 0;
    }

    if (pCtrl->qavScene.index >= 0) {
        pCtrl->qavScene.index = -1;
        pCtrl->qavScene.qavResrc = NULL;
        pPlayer->sceneQav = -1;

        // restore weapon
        if (pPlayer->pXSprite->health > 0) {
            int oldWeapon = (pXSource && pXSource->dropMsg != 0) ? pXSource->dropMsg : 1;
            pPlayer->newWeapon = pPlayer->curWeapon = oldWeapon;
            WeaponRaise(pPlayer);
        }
    }

}

void trPlayerCtrlLink(XSPRITE* pXSource, PLAYER* pPlayer, bool checkCondition) {

    // save player's sprite index to let the tracking condition know it after savegame loading...
    pXSource->sysData1                  = pPlayer->nSprite;
    
    pPlayer->pXSprite->txID             = pXSource->txID;
    pPlayer->pXSprite->command          = kCmdToggle;
    pPlayer->pXSprite->triggerOn        = pXSource->triggerOn;
    pPlayer->pXSprite->triggerOff       = pXSource->triggerOff;
    pPlayer->pXSprite->busyTime         = pXSource->busyTime;
    pPlayer->pXSprite->waitTime         = pXSource->waitTime;
    pPlayer->pXSprite->restState        = pXSource->restState;

    pPlayer->pXSprite->Push             = pXSource->Push;
    pPlayer->pXSprite->Impact           = pXSource->Impact;
    pPlayer->pXSprite->Vector           = pXSource->Vector;
    pPlayer->pXSprite->Touch            = pXSource->Touch;
    pPlayer->pXSprite->Sight            = pXSource->Sight;
    pPlayer->pXSprite->Proximity        = pXSource->Proximity;

    pPlayer->pXSprite->Decoupled        = pXSource->Decoupled;
    pPlayer->pXSprite->Interrutable     = pXSource->Interrutable;
    pPlayer->pXSprite->DudeLockout      = pXSource->DudeLockout;

    pPlayer->pXSprite->data1            = pXSource->data1;
    pPlayer->pXSprite->data2            = pXSource->data2;
    pPlayer->pXSprite->data3            = pXSource->data3;
    pPlayer->pXSprite->data4            = pXSource->data4;

    pPlayer->pXSprite->key              = pXSource->key;
    pPlayer->pXSprite->dropMsg          = pXSource->dropMsg;

    // let's check if there is tracking condition expecting objects with this TX id
    if (checkCondition && pXSource->txID >= kChannelUser) {
        for (int i = 0; i < gTrackingCondsCount; i++) {
            
            TRCONDITION* pCond = &gCondition[i];
            if (xsprite[pCond->xindex].rxID != pXSource->txID)
                continue;
                
            // search for player control sprite and replace it with actual player sprite
            for (unsigned k = 0; k < pCond->length; k++) {
                if (pCond->obj[k].type != OBJ_SPRITE || pCond->obj[k].index != pXSource->reference) continue;
                pCond->obj[k].index = pPlayer->nSprite;
                pCond->obj[k].cmd = pPlayer->pXSprite->command;
                break;
            }

        }
    }
}

void trPlayerCtrlSetRace(XSPRITE* pXSource, PLAYER* pPlayer) {
    playerSetRace(pPlayer, pXSource->data2);
    switch (pPlayer->lifeMode) {
        case kModeHuman:
        case kModeBeast:
            playerSizeReset(pPlayer);
            break;
        case kModeHumanShrink:
            playerSizeShrink(pPlayer, 2);
            break;
        case kModeHumanGrown:
            playerSizeGrow(pPlayer, 2);
            break;
    }
}

void trPlayerCtrlSetMoveSpeed(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    int speed = ClipRange(pXSource->data2, 0, 500);
    for (int i = 0; i < kModeMax; i++) {
        for (int a = 0; a < kPostureMax; a++) {
            POSTURE* curPosture = &pPlayer->pPosture[i][a]; POSTURE* defPosture = &gPostureDefaults[i][a];
            curPosture->frontAccel = (defPosture->frontAccel * speed) / kPercFull;
            curPosture->sideAccel = (defPosture->sideAccel * speed) / kPercFull;
            curPosture->backAccel = (defPosture->backAccel * speed) / kPercFull;
        }
    }
}

void trPlayerCtrlSetJumpHeight(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    int jump = ClipRange(pXSource->data3, 0, 500);
    for (int i = 0; i < kModeMax; i++) {
        POSTURE* curPosture = &pPlayer->pPosture[i][kPostureStand]; POSTURE* defPosture = &gPostureDefaults[i][kPostureStand];
        curPosture->normalJumpZ = (defPosture->normalJumpZ * jump) / kPercFull;
        curPosture->pwupJumpZ = (defPosture->pwupJumpZ * jump) / kPercFull;
    }
}

void trPlayerCtrlSetScreenEffect(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    int eff = ClipLow(pXSource->data2, 0); int time = (eff > 0) ? pXSource->data3 : 0;
    switch (eff) {
        case 0: // clear all
        case 1: // tilting
            pPlayer->tiltEffect = ClipRange(time, 0, 220);
            if (eff) break;
            fallthrough__;
        case 2: // pain
            pPlayer->painEffect = ClipRange(time, 0, 2048);
            if (eff) break;
            fallthrough__;
        case 3: // blind
            pPlayer->blindEffect = ClipRange(time, 0, 2048);
            if (eff) break;
            fallthrough__;
        case 4: // pickup
            pPlayer->pickupEffect = ClipRange(time, 0, 2048);
            if (eff) break;
            fallthrough__;
        case 5: // quakeEffect
            pPlayer->quakeEffect = ClipRange(time, 0, 2048);
            if (eff) break;
            fallthrough__;
        case 6: // visibility
            pPlayer->visibility = ClipRange(time, 0, 2048);
            if (eff) break;
            fallthrough__;
        case 7: // delirium
            pPlayer->pwUpTime[kPwUpDeliriumShroom] = ClipHigh(time << 1, 432000);
            break;
    }

}

void trPlayerCtrlSetLookAngle(XSPRITE* pXSource, PLAYER* pPlayer)
{
    int look = pXSource->data2 << 5;

    if (abs(look) > 0)
    {
        if (pPlayer->horizon.horiz.asq16() != 0)
        {
            // move horiz back to 0
            pPlayer->horizon.horiz += q16horiz(xs_CRoundToInt(-pPlayer->horizon.horiz.asq16() * (1. / 3.)));
        }
    }
    else
    {
        pPlayer->horizon.horiz = q16horiz(0);
    }

}

void trPlayerCtrlEraseStuff(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    switch (pXSource->data2) {
        case 0: // erase all
            fallthrough__;
        case 1: // erase weapons
            WeaponLower(pPlayer);

            for (int i = 0; i < 14; i++) {
                pPlayer->hasWeapon[i] = false;
                // also erase ammo
                if (i < 12) pPlayer->ammoCount[i] = 0;
            }

            pPlayer->hasWeapon[1] = true;
            pPlayer->curWeapon = 0;
            pPlayer->nextWeapon = 1;

            WeaponRaise(pPlayer);
            if (pXSource->data2) break;
            fallthrough__;
        case 2: // erase all armor
            for (int i = 0; i < 3; i++) pPlayer->armor[i] = 0;
            if (pXSource->data2) break;
            fallthrough__;
        case 3: // erase all pack items
            for (int i = 0; i < 5; i++) {
                pPlayer->packSlots[i].isActive = false;
                pPlayer->packSlots[i].curAmount = 0;
            }
            pPlayer->packItemId = -1;
            if (pXSource->data2) break;
            fallthrough__;
        case 4: // erase all keys
            for (int i = 0; i < 8; i++) pPlayer->hasKey[i] = false;
            if (pXSource->data2) break;
            fallthrough__;
        case 5: // erase powerups
            for (int i = 0; i < kMaxPowerUps; i++) pPlayer->pwUpTime[i] = 0;
            break;
    }

}

void trPlayerCtrlGiveStuff(XSPRITE* pXSource, PLAYER* pPlayer, TRPLAYERCTRL* pCtrl) {
    
    int weapon = pXSource->data3;
    switch (pXSource->data2) {
        case 1: // give N weapon and default ammo for it
        case 2: // give just N ammo for selected weapon
            if (weapon <= 0 || weapon > 13) {
                Printf(PRINT_HIGH, "Weapon #%d is out of a weapons range!", weapon);
                break;
            } else if (pXSource->data2 == 2 && pXSource->data4 == 0) {
                Printf(PRINT_HIGH, "Zero ammo for weapon #%d is specified!", weapon);
                break;
            }
            switch (weapon) {
                case 11: // remote bomb 
                case 12: // prox bomb
                    pPlayer->hasWeapon[weapon] = true;
                    weapon--;
                    pPlayer->ammoCount[weapon] = ClipHigh(pPlayer->ammoCount[weapon] + ((pXSource->data2 == 2) ? pXSource->data4 : 1), gAmmoInfo[weapon].max);
                    weapon++;
                    break;
                default:
                    for (int i = 0; i < 11; i++) {
                        if (gWeaponItemData[i].type != weapon) continue;

                        const WEAPONITEMDATA* pWeaponData = &gWeaponItemData[i]; 
                        int nAmmoType = pWeaponData->ammoType;
                        switch (pXSource->data2) {
                            case 1:
                                pPlayer->hasWeapon[weapon] = true;
                                if (pPlayer->ammoCount[nAmmoType] >= pWeaponData->count) break;
                                pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponData->count, gAmmoInfo[nAmmoType].max);
                                break;
                            case 2:
                                pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pXSource->data4, gAmmoInfo[nAmmoType].max);
                                break;
                        }
                        break;
                    }
                    break;
            }
            if (pPlayer->hasWeapon[weapon] && pXSource->data4 == 0) { // switch on it
                pPlayer->nextWeapon = 0;

                if (pPlayer->sceneQav >= 0 && spriRangeIsFine(pCtrl->qavScene.index)) {
                    XSPRITE* pXScene = &xsprite[sprite[pCtrl->qavScene.index].extra];
                    pXScene->dropMsg = weapon;
                } else if (pPlayer->curWeapon != weapon) {
                    pPlayer->newWeapon = weapon;
                    WeaponRaise(pPlayer);
                }
            }
            break;
    }
}

void trPlayerCtrlUsePackItem(XSPRITE* pXSource, PLAYER* pPlayer, int evCmd) {
    unsigned int invItem = pXSource->data2 - 1;
    switch (evCmd) {
        case kCmdOn:
            if (!pPlayer->packSlots[invItem].isActive) packUseItem(pPlayer, invItem);
            break;
        case kCmdOff:
            if (pPlayer->packSlots[invItem].isActive) packUseItem(pPlayer, invItem);
            break;
        default:
            packUseItem(pPlayer, invItem);
            break;
    }

    switch (pXSource->data4) {
        case 2: // both
        case 0: // switch on it
            if (pPlayer->packSlots[invItem].curAmount > 0) pPlayer->packItemId = invItem;
            if (!pXSource->data4) break;
            fallthrough__;
        case 1: // force remove after use
            pPlayer->packSlots[invItem].isActive = false;
            pPlayer->packSlots[invItem].curAmount = 0;
            break;
    }
}

void useObjResizer(XSPRITE* pXSource, short objType, int objIndex) {
    switch (objType) {
    // for sectors
    case 6:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            sector[objIndex].floorxpan_ = (float)ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            sector[objIndex].floorypan_ = (float)ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sector[objIndex].ceilingxpan_ = (float)ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sector[objIndex].ceilingypan_ = (float)ClipRange(pXSource->data4, 0, 255);
        break;
    // for sprites
    case OBJ_SPRITE: {

        bool fit = false;
        // resize by seq scaling
        if (sprite[pXSource->reference].flags & kModernTypeFlag1) {
            
            if (valueIsBetween(pXSource->data1, -255, 32767)) {
                int mulDiv = (valueIsBetween(pXSource->data2, 0, 257)) ? pXSource->data2 : 256;
                if (pXSource->data1 > 0) xsprite[sprite[objIndex].extra].scale = mulDiv * ClipHigh(pXSource->data1, 25);
                else if (pXSource->data1 < 0) xsprite[sprite[objIndex].extra].scale = mulDiv / ClipHigh(abs(pXSource->data1), 25);
                else xsprite[sprite[objIndex].extra].scale = 0;
                fit = true;
                }

        // resize by repeats
        } else {

            if (valueIsBetween(pXSource->data1, -1, 32767)) {
                sprite[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);
                fit = true;
            }
            
            if (valueIsBetween(pXSource->data2, -1, 32767)) {
                sprite[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);
                fit = true;
            }

        }

        if (fit && (sprite[objIndex].type == kDudeModernCustom || sprite[objIndex].type == kDudeModernCustomBurning)) {
            
            // request properties update for custom dude
            gGenDudeExtra[objIndex].updReq[kGenDudePropertySpriteSize] = true;
            gGenDudeExtra[objIndex].updReq[kGenDudePropertyAttack] = true;
            gGenDudeExtra[objIndex].updReq[kGenDudePropertyMass] = true;
            gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
            evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);

        }

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sprite[objIndex].xoffset = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sprite[objIndex].yoffset = ClipRange(pXSource->data4, 0, 255);
        break;
    }
    case OBJ_WALL:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            wall[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            wall[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            wall[objIndex].xpan_ = (float)ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            wall[objIndex].ypan_ = (float)ClipRange(pXSource->data4, 0, 255);
        break;
    }

}

void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex) {

    spritetype* pSource = &sprite[pXSource->reference];

    switch (objType) {
        case OBJ_WALL: {
            walltype* pWall = &wall[objIndex]; int old = -1;

            // data3 = set wall hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) pWall->hitag = pWall->hitag |= pXSource->data3;
                else pWall->hitag = pXSource->data3;
            }

            // data4 = set wall cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                old = pWall->cstat;

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pWall->cstat = pWall->cstat |= pXSource->data4; // relative
                else pWall->cstat = pXSource->data4; // absolute

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
        }
        break;
        case OBJ_SPRITE: {
            spritetype* pSprite = &sprite[objIndex]; bool thing2debris = false;
            XSPRITE* pXSprite = &xsprite[pSprite->extra]; int old = -1;

            // data3 = set sprite hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                old = pSprite->flags;

                // set new hitag
                if ((pSource->flags & kModernTypeFlag1)) pSprite->flags = pSource->flags |= pXSource->data3; // relative
                else pSprite->flags = pXSource->data3;  // absolute

                // and handle exceptions
                if ((old & kHitagFree) && !(pSprite->flags & kHitagFree)) pSprite->flags |= kHitagFree;
                if ((old & kHitagRespawn) && !(pSprite->flags & kHitagRespawn)) pSprite->flags |= kHitagRespawn;

                // prepare things for different (debris) physics.
                if (pSprite->statnum == kStatThing && debrisGetFreeIndex() >= 0) thing2debris = true;

            }

            // data2 = sprite physics settings
            if (valueIsBetween(pXSource->data2, -1, 32767) || thing2debris) {
                switch (pSprite->statnum) {
                case kStatDude: // dudes already treating in game
                case kStatFree:
                case kStatMarker:
                case kStatPathMarker:
                    break;
                default:
                    // store physics attributes in xsprite to avoid setting hitag for modern types!
                    int flags = (pXSprite->physAttr != 0) ? pXSprite->physAttr : 0;
                    int oldFlags = flags;

                    if (thing2debris) {

                        // converting thing to debris
                        if ((pSprite->flags & kPhysMove) != 0) flags |= kPhysMove;
                        else flags &= ~kPhysMove;

                        if ((pSprite->flags & kPhysGravity) != 0) flags |= (kPhysGravity | kPhysFalling);
                        else flags &= ~(kPhysGravity | kPhysFalling);

                        pSprite->flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
                        xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;
                        pXSprite->restState = pXSprite->state;

                    } else {

                        static char digits[6];
                        memset(digits, 0, sizeof(digits));
                        sprintf(digits, "%d", pXSource->data2);
                        for (int i = 0; i < sizeof(digits); i++)
                            digits[i] = (digits[i] >= 48 && digits[i] <= 57) ? (digits[i] - 57) + 9 : 0;

                        // first digit of data2: set main physics attributes
                        switch (digits[0]) {
                            case 0:
                                flags &= ~kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;
                            case 1:
                                flags |= kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;
                            case 2:
                                flags &= ~kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                            case 3:
                                flags |= kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                        }

                        // second digit of data2: touch physics flags
                        switch (digits[1]) {
                            case 0:
                                flags &= ~kPhysDebrisTouch;
                                break;
                            case 1:
                                flags |= kPhysDebrisTouch;
                                break;
                        }

                        // third digit of data2: weapon physics flags
                        switch (digits[2]) {
                            case 0:
                                flags &= ~kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;
                            case 1:
                                flags |= kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;
                            case 2:
                                flags &= ~kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                            case 3:
                                flags |= kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                        }

                        // fourth digit of data2: swimming / flying physics flags
                        switch (digits[3]) {
                            case 0:
                                flags &= ~kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 1:
                                flags |= kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 2:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFloat;
                                flags &= ~kPhysDebrisFly;
                                break;
                            case 3:
                                flags |= kPhysDebrisFly;
                                flags &= ~kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 4:
                                flags |= kPhysDebrisFly;
                                flags |= kPhysDebrisFloat;
                                flags &= ~kPhysDebrisSwim;
                                break;
                            case 5:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 6:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFly;
                                flags |= kPhysDebrisFloat;
                                break;
                    }

                    }

                    int nIndex = debrisGetIndex(objIndex); // check if there is no sprite in list

                    // adding physics sprite in list
                    if ((flags & kPhysGravity) != 0 || (flags & kPhysMove) != 0) {

                        if (oldFlags == 0)
                            xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;

                        if (nIndex != -1) {

                            pXSprite->physAttr = flags; // just update physics attributes

                        } else if ((nIndex = debrisGetFreeIndex()) < 0) {
                            
                            viewSetSystemMessage("Max (%d) Physics affected sprites reached!", kMaxSuperXSprites);

                        } else {

                            pXSprite->physAttr = flags; // update physics attributes

                            // allow things to became debris, so they use different physics...
                            if (pSprite->statnum == kStatThing) changespritestat(objIndex, 0);

                            // set random goal ang for swimming so they start turning
                            if ((flags & kPhysDebrisSwim) && !xvel[objIndex] && !yvel[objIndex] && !zvel[objIndex])
                                pXSprite->goalAng = (pSprite->ang + Random3(kAng45)) & 2047;
                            
                            if (pXSprite->physAttr & kPhysDebrisVector)
                                pSprite->cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;

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

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pSprite->cstat |= pXSource->data4; // relative
                else pSprite->cstat = pXSource->data4; // absolute

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
        }
        break;
        case OBJ_SECTOR: {

            sectortype* pSector = &sector[objIndex];
            XSECTOR* pXSector = &xsector[sector[objIndex].extra];

            // data1 = sector underwater status and depth level
            if (pXSource->data1 >= 0 && pXSource->data1 < 2) {
                
                pXSector->Underwater = (pXSource->data1) ? true : false;

                spritetype* pLower = (gLowerLink[objIndex] >= 0) ? &sprite[gLowerLink[objIndex]] : NULL;
                XSPRITE* pXLower = NULL; spritetype* pUpper = NULL; XSPRITE* pXUpper = NULL;
                
                if (pLower) {
                    
                    pXLower = &xsprite[pLower->extra];

                    // must be sure we found exact same upper link
                    for (int i = 0; i < kMaxSectors; i++) {
                        if (gUpperLink[i] < 0 || xsprite[sprite[gUpperLink[i]].extra].data1 != pXLower->data1) continue;
                        pUpper = &sprite[gUpperLink[i]]; pXUpper = &xsprite[pUpper->extra];
                        break;
                    }
                }
                
                // treat sectors that have links, so warp can detect underwater status properly
                if (pLower) {
                    if (pXSector->Underwater) {
                        switch (pLower->type) {
                            case kMarkerLowStack:
                            case kMarkerLowLink:
                                pXLower->sysData1 = pLower->type;
                                pLower->type = kMarkerLowWater;
                                break;
                            default:
                                if (pSector->ceilingpicnum < 4080 || pSector->ceilingpicnum > 4095) pXLower->sysData1 = kMarkerLowLink;
                                else pXLower->sysData1 = kMarkerLowStack;
                                break;
                        }
                    }
                    else if (pXLower->sysData1 > 0) pLower->type = pXLower->sysData1;
                    else if (pSector->ceilingpicnum < 4080 || pSector->ceilingpicnum > 4095) pLower->type = kMarkerLowLink;
                    else pLower->type = kMarkerLowStack;
                }

                if (pUpper) {
                    if (pXSector->Underwater) {
                        switch (pUpper->type) {
                            case kMarkerUpStack:
                            case kMarkerUpLink:
                                pXUpper->sysData1 = pUpper->type;
                                pUpper->type = kMarkerUpWater;
                                break;
                            default:
                                if (pSector->floorpicnum < 4080 || pSector->floorpicnum > 4095) pXUpper->sysData1 = kMarkerUpLink;
                                else pXUpper->sysData1 = kMarkerUpStack;
                                break;
                        }
                    }
                    else if (pXUpper->sysData1 > 0) pUpper->type = pXUpper->sysData1;
                    else if (pSector->floorpicnum < 4080 || pSector->floorpicnum > 4095) pUpper->type = kMarkerUpLink;
                    else pUpper->type = kMarkerUpStack;
                }

                // search for dudes in this sector and change their underwater status
                int nSprite;
                SectIterator it(objIndex);
                while ((nSprite = it.NextIndex()) >= 0)
                {
                    spritetype* pSpr = &sprite[nSprite];
                    if (pSpr->statnum != kStatDude || !IsDudeSprite(pSpr) || !xspriRangeIsFine(pSpr->extra))
                        continue;

                    PLAYER* pPlayer = getPlayerById(pSpr->type);
                    if (pXSector->Underwater) {
                        if (pLower)
                            xsprite[pSpr->extra].medium = (pLower->type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

                        if (pPlayer) {
                            int waterPal = kMediumWater;
                            if (pLower) {
                                if (pXLower->data2 > 0) waterPal = pXLower->data2;
                                else if (pLower->type == kMarkerUpGoo) waterPal = kMediumGoo;
                            }

                            pPlayer->nWaterPal = waterPal;
                            pPlayer->posture = kPostureSwim;
                            pPlayer->pXSprite->burnTime = 0;
                        }

                    } else {

                        xsprite[pSpr->extra].medium = kMediumNormal;
                        if (pPlayer) {
                            pPlayer->posture = (!(pPlayer->input.actions & SB_CROUCH)) ? kPostureStand : kPostureCrouch;
                            pPlayer->nWaterPal = 0;
                        }

                    }
                }
            }
            else if (pXSource->data1 > 9) pXSector->Depth = 7;
            else if (pXSource->data1 > 1) pXSector->Depth = pXSource->data1 - 2;


            // data2 = sector visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].visibility = ClipRange(pXSource->data2, 0 , 234);

            // data3 = sector ceil cstat
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].ceilingstat |= pXSource->data3;
                else sector[objIndex].ceilingstat = pXSource->data3;
            }

            // data4 = sector floor cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].floorstat |= pXSource->data4;
                else sector[objIndex].floorstat = pXSource->data4;
            }
        }
        break;
        // no TX id
        case -1:
            // data2 = global visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                gVisibility = ClipRange(pXSource->data2, 0, 4096);
        break;
    }

}

void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference]; PLAYER* pPlayer = getPlayerById(pSprite->type);
    XSECTOR* pXSector = (sector[pSource->sectnum].extra >= 0) ? &xsector[sector[pSource->sectnum].extra] : NULL;
    bool isDude = (!pPlayer && IsDudeSprite(pSprite));

    if (pSprite->sectnum != pSource->sectnum)
        changespritesect(pSprite->index, pSource->sectnum);

    pSprite->x = pSource->x; pSprite->y = pSource->y; pSprite->z = pSource->z;
    
    // make sure sprites aren't in the floor or ceiling
    int zTop, zBot; GetSpriteExtents(pSprite, &zTop, &zBot);
    pSprite->z += ClipLow(sector[pSprite->sectnum].ceilingz - zTop, 0);
    pSprite->z += ClipHigh(sector[pSprite->sectnum].floorz - zBot, 0);

    if (pSource->flags & kModernTypeFlag1) // force telefrag
        TeleFrag(pSprite->index, pSource->sectnum);


    if (pSprite->flags & kPhysGravity)
        pSprite->flags |= kPhysFalling;

    if (pXSector) {

        if (pXSector->Enter && (pPlayer || (isDude && !pXSector->dudeLockout)))
            trTriggerSector(pSource->sectnum, pXSector, kCmdSectorEnter);

        if (pXSector->Underwater) {
            spritetype* pLink = (gLowerLink[pSource->sectnum] >= 0) ? &sprite[gLowerLink[pSource->sectnum]] : NULL;
            if (pLink) {

                // must be sure we found exact same upper link
                for (int i = 0; i < kMaxSectors; i++) {
                    if (gUpperLink[i] < 0 || xsprite[sprite[gUpperLink[i]].extra].data1 != xsprite[pLink->extra].data1) continue;
                    pLink = &sprite[gUpperLink[i]];
                    break;
                }

            }

            if (pLink)
                xsprite[pSprite->extra].medium = (pLink->type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

            if (pPlayer) {
                int waterPal = kMediumWater;
                if (pLink) {
                    if (xsprite[pLink->extra].data2 > 0) waterPal = xsprite[pLink->extra].data2;
                    else if (pLink->type == kMarkerUpGoo) waterPal = kMediumGoo;
                }

                pPlayer->nWaterPal = waterPal;
                pPlayer->posture = kPostureSwim;
                pPlayer->pXSprite->burnTime = 0;
            }

        } else {

            xsprite[pSprite->extra].medium = kMediumNormal;
            if (pPlayer) {
                pPlayer->posture = (!(pPlayer->input.actions & SB_CROUCH)) ? kPostureStand : kPostureCrouch;
                pPlayer->nWaterPal = 0;
            }

        }
    }

    if (pSprite->statnum == kStatDude && IsDudeSprite(pSprite) && !IsPlayerSprite(pSprite)) {
        XSPRITE* pXDude = &xsprite[pSprite->extra];
        int x = pXDude->targetX; int y = pXDude->targetY; int z = pXDude->targetZ;
        int target = pXDude->target;
        
        aiInitSprite(pSprite);

        if (target >= 0) {
            pXDude->targetX = x; pXDude->targetY = y; pXDude->targetZ = z;
            pXDude->target = target; aiActivateDude(&bloodActors[pXDude->reference]);
        }
    }

    if (pXSource->data2 == 1) {
        
        if (pPlayer) pPlayer->angle.ang = buildang(pSource->ang);
        else if (isDude) xsprite[pSprite->extra].goalAng = pSprite->ang = pSource->ang;
        else pSprite->ang = pSource->ang;
    }

    if (pXSource->data3 == 1)
        xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;

    viewBackupSpriteLoc(pSprite->index, pSprite);

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSource, pXSource->data4, -1, 0);

    if (pPlayer) {
        playerResetInertia(pPlayer);
        if (pXSource->data2 == 1)
            pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
    }

}


void useEffectGen(XSPRITE* pXSource, spritetype* pSprite) {
    if (pSprite == NULL) pSprite = &sprite[pXSource->reference];
    int fxId = (pXSource->data3 <= 0) ? pXSource->data2 : pXSource->data2 + Random(pXSource->data3 + 1);
    if (xspriRangeIsFine(pSprite->extra) && valueIsBetween(fxId, 0, kFXMax)) {
        int pos, top, bottom; GetSpriteExtents(pSprite, &top, &bottom);
        spritetype* pSource = &sprite[pXSource->reference];
        spritetype* pEffect = NULL;

        // select where exactly effect should be spawned
        switch (pXSource->data4) {
            case 1:
                pos = bottom;
                break;
            case 2: // middle
                pos = pSprite->z + (tileHeight(pSprite->picnum) / 2 + tileTopOffset(pSprite->picnum));
                break;
            case 3:
            case 4:
                if (!sectRangeIsFine(pSprite->sectnum)) pos = top;
                else pos = (pXSource->data4 == 3) ? sector[pSprite->sectnum].floorz : sector[pSprite->sectnum].ceilingz;
                break;
            default:
                pos = top;
                break;
        }

        if ((pEffect = gFX.fxSpawn((FX_ID)fxId, pSprite->sectnum, pSprite->x, pSprite->y, pos, 0)) != NULL) {

            pEffect->owner = pSource->index;

            if (pSource->flags & kModernTypeFlag1) {
                pEffect->pal = pSource->pal;
                pEffect->xoffset = pSource->xoffset;
                pEffect->yoffset = pSource->yoffset;
                pEffect->xrepeat = pSource->xrepeat;
                pEffect->yrepeat = pSource->yrepeat;
                pEffect->shade = pSource->shade;
            }

            if (pSource->flags & kModernTypeFlag2) {
                pEffect->cstat = pSource->cstat;
                if (pEffect->cstat & CSTAT_SPRITE_INVISIBLE)
                    pEffect->cstat &= ~CSTAT_SPRITE_INVISIBLE;
            }

            if (pEffect->cstat & CSTAT_SPRITE_ONE_SIDED)
                pEffect->cstat &= ~CSTAT_SPRITE_ONE_SIDED;

        }
    }
}


void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector) {

    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = NULL; int nXSector = 0;
    
    if (pSector != NULL) {
        pXSector = &xsector[pSector->extra];
        nXSector = sector[pXSector->reference].extra;
    } else if (xsectRangeIsFine(sector[pSource->sectnum].extra)) {
        pXSector = &xsector[sector[pSource->sectnum].extra];
        nXSector = sector[pXSector->reference].extra;
    } else {
        int nXSector = dbInsertXSector(pSource->sectnum);
        pXSector = &xsector[nXSector]; pXSector->windAlways = 1;
    }

    if ((pSource->flags & kModernTypeFlag1))
        pXSector->panAlways = pXSector->windAlways = 1;

    short windVel = ClipRange(pXSource->data2, 0, 32767);
    switch (pXSource->data1) {
        default:
            pXSector->windVel = windVel;
            break;
        case 1:
        case 3:
            pXSector->windVel = nnExtRandom(0, windVel);
            break;
    }
    
    int ang = pSource->ang;
    if (pXSource->data4 <= 0) {
        switch (pXSource->data1) {
        case 2:
        case 3:
            while (pSource->ang == ang)
                pSource->ang = nnExtRandom(-kAng360, kAng360) & 2047;
            break;
        }
    }
    else if (pSource->cstat & 0x2000) pSource->ang += pXSource->data4;
    else if (pSource->cstat & 0x4000) pSource->ang -= pXSource->data4;
    else if (pXSource->sysData1 == 0) {
        if ((ang += pXSource->data4) >= kAng180) pXSource->sysData1 = 1;
        pSource->ang = ClipHigh(ang, kAng180);
    } else {
        if ((ang -= pXSource->data4) <= -kAng180) pXSource->sysData1 = 0;
        pSource->ang = ClipLow(ang, -kAng180);
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
        if (pXSector->panCeiling)
        {
            StartInterpolation(pXSector->reference, Interp_Sect_CeilingPanX);
            StartInterpolation(pXSector->reference, Interp_Sect_CeilingPanY);
        }
        if (pXSector->panFloor)
        {
            StartInterpolation(pXSector->reference, Interp_Sect_FloorPanX);
            StartInterpolation(pXSector->reference, Interp_Sect_FloorPanY);
        }

        short oldPan = pXSector->panVel;
        pXSector->panAngle = pXSector->windAng;
        pXSector->panVel = pXSector->windVel;

        // add to panList if panVel was set to 0 previously
        if (oldPan == 0 && pXSector->panVel != 0 && panCount < kMaxXSectors) {

            int i;
            for (i = 0; i < panCount; i++) {
                if (panList[i] != nXSector) continue;
                break;
            }

            if (i == panCount)
            {
                panList[panCount++] = nXSector;
            }
        }

    }
}

void useSpriteDamager(XSPRITE* pXSource, int objType, int objIndex) {

    spritetype* pSource = &sprite[pXSource->reference];
    sectortype* pSector = &sector[pSource->sectnum];

    int top, bottom, i;
    bool floor, ceil, wall, enter;

    switch (objType) {
        case OBJ_SPRITE:
            damageSprites(pXSource, &sprite[objIndex]);
            break;
        case OBJ_SECTOR:
            GetSpriteExtents(pSource, &top, &bottom);
            floor = (bottom >= pSector->floorz);    ceil = (top <= pSector->ceilingz);
            wall = (pSource->cstat & 0x10);         enter = (!floor && !ceil && !wall);
            for (i = headspritesect[objIndex]; i != -1; i = nextspritesect[i]) {
                if (!IsDudeSprite(&sprite[i]) || !xspriRangeIsFine(sprite[i].extra))
                    continue;
                else if (enter)
                    damageSprites(pXSource, &sprite[i]);
                else if (floor && (gSpriteHit[sprite[i].extra].florhit & 0xc000) == 0x4000 && (gSpriteHit[sprite[i].extra].florhit & 0x3fff) == objIndex)
                    damageSprites(pXSource, &sprite[i]);
                else if (ceil && (gSpriteHit[sprite[i].extra].ceilhit & 0xc000) == 0x4000 && (gSpriteHit[sprite[i].extra].ceilhit & 0x3fff) == objIndex)
                    damageSprites(pXSource, &sprite[i]);
                else if (wall && (gSpriteHit[sprite[i].extra].hit & 0xc000) == 0x8000 && sectorofwall(gSpriteHit[sprite[i].extra].hit & 0x3fff) == objIndex)
                    damageSprites(pXSource, &sprite[i]);
            }
            break;
        case -1:
            for (i = headspritestat[kStatDude]; i != -1; i = nextspritestat[i]) {
                if (sprite[i].statnum != kStatDude) continue;
                switch (pXSource->data1) {
                    case 667:
                        if (IsPlayerSprite(&sprite[i])) continue;
                        damageSprites(pXSource, &sprite[i]);
                        break;
                    case 668:
                        if (!IsPlayerSprite(&sprite[i])) continue;
                        damageSprites(pXSource, &sprite[i]);
                        break;
                    default:
                        damageSprites(pXSource, &sprite[i]);
                        break;
                }
            }
            break;
    }
}

void damageSprites(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference];
    if (!IsDudeSprite(pSprite) || !xspriRangeIsFine(pSprite->extra) || xsprite[pSprite->extra].health <= 0 || pXSource->data3 < 0)
        return;
    

    XSPRITE* pXSprite = &xsprite[pSprite->extra]; PLAYER* pPlayer = getPlayerById(pSprite->type);
    int dmgType = (pXSource->data2 >= kDmgFall) ? ClipHigh(pXSource->data2, kDmgElectric) : -1;
    int dmg = pXSprite->health << 4; int armor[3];

    bool godMode = (pPlayer && ((dmgType >= 0 && pPlayer->damageControl[dmgType]) || powerupCheck(pPlayer, kPwUpDeathMask) || pPlayer->godMode)); // kneeling

    if (godMode || pXSprite->locked) return;
    else if (pXSource->data3) {
        if (pSource->flags & kModernTypeFlag1) dmg = ClipHigh(pXSource->data3 << 1, 65535);
        else if (pXSprite->sysData2 > 0) dmg = (ClipHigh(pXSprite->sysData2 << 4, 65535) * pXSource->data3) / kPercFull;
        else dmg = ((getDudeInfo(pSprite->type)->startHealth << 4) * pXSource->data3) / kPercFull;
    }

    if (dmgType >= kDmgFall) {
        if (dmg < (int)pXSprite->health << 4) {
            if (nnExtIsImmune(pSprite, dmgType, 0)) {
                Printf(PRINT_HIGH, "Dude type %d is immune to damage type %d!", pSprite->type, dmgType);
                return;
            }

            if (pPlayer) {
                
                playerDamageArmor(pPlayer, (DAMAGE_TYPE)dmgType, dmg);
                for (int i = 0; i < 3; armor[i] = pPlayer->armor[i], pPlayer->armor[i] = 0, i++);
                actDamageSprite(pSource->index, pSprite, (DAMAGE_TYPE)dmgType, dmg);
                for (int i = 0; i < 3; pPlayer->armor[i] = armor[i], i++);

            } else {
                
                actDamageSprite(pSource->index, pSprite, (DAMAGE_TYPE)dmgType, dmg);

            }

        }
        else if (!pPlayer) actKillDude(pSource->index, pSprite, (DAMAGE_TYPE)dmgType, dmg);
        else playerDamageSprite(&bloodActors[pSource->index], pPlayer, (DAMAGE_TYPE)dmgType, dmg);
    }
    else if ((pXSprite->health = ClipLow(pXSprite->health - dmg, 1)) > 16) return;
    else if (!pPlayer) actKillDude(pSource->index, pSprite, DAMAGE_TYPE_2, dmg);
    else playerDamageSprite(&bloodActors[pSource->index], pPlayer, DAMAGE_TYPE_2, dmg);
    return;
}

void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index) {

    if (pXSource->data2 > 0 && !getSequence(pXSource->data2)) {
        Printf(PRINT_HIGH, "Missing sequence #%d", pXSource->data2);
        return;
    }

    spritetype* pSource = &sprite[pXSource->reference];
    switch (objType) {
        case OBJ_SECTOR:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(2, sector[index].extra);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqKill(1, sector[index].extra);
            } else {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 2, sector[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqSpawn(pXSource->data2, 1, sector[index].extra, -1);
            }
            return;
        case OBJ_WALL:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(0, wall[index].extra);
                if ((pXSource->data3 == 3 || pXSource->data3 == 2) && (wall[index].cstat & CSTAT_WALL_MASKED))
                    seqKill(4, wall[index].extra);
            } else {

                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 0, wall[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2) {

                    if (wall[index].nextwall < 0) {
                        if (pXSource->data3 == 3)
                            seqSpawn(pXSource->data2, 0, wall[index].extra, -1);

                    } else {
                        if (!(wall[index].cstat & CSTAT_WALL_MASKED))
                            wall[index].cstat |= CSTAT_WALL_MASKED;

                        seqSpawn(pXSource->data2, 4, wall[index].extra, -1);
                    }
                }

                if (pXSource->data4 > 0) {

                    int cx, cy, cz;
                    cx = (wall[index].x + wall[wall[index].point2].x) >> 1;
                    cy = (wall[index].y + wall[wall[index].point2].y) >> 1;
                    int nSector = sectorofwall(index);
                    int32_t ceilZ, floorZ;
                    getzsofslope(nSector, cx, cy, &ceilZ, &floorZ);
                    int32_t ceilZ2, floorZ2;
                    getzsofslope(wall[index].nextsector, cx, cy, &ceilZ2, &floorZ2);
                    ceilZ = ClipLow(ceilZ, ceilZ2);
                    floorZ = ClipHigh(floorZ, floorZ2);
                    cz = (ceilZ + floorZ) >> 1;

                    sfxPlay3DSound(cx, cy, cz, pXSource->data4, nSector);

                }

            }
            return;
        case OBJ_SPRITE:
            if (pXSource->data2 <= 0) seqKill(3, sprite[index].extra);
            else if (sectRangeIsFine(sprite[index].sectnum)) {
                    if (pXSource->data3 > 0) {
                        int nSprite = InsertSprite(sprite[index].sectnum, kStatDecoration);
                        int top, bottom; GetSpriteExtents(&sprite[index], &top, &bottom);
                        sprite[nSprite].x = sprite[index].x;
                        sprite[nSprite].y = sprite[index].y;
                        switch (pXSource->data3) {
                            default:
                                sprite[nSprite].z = sprite[index].z;
                                break;
                            case 2:
                                sprite[nSprite].z = bottom;
                                break;
                            case 3:
                                sprite[nSprite].z = top;
                                break;
                            case 4:
                                sprite[nSprite].z = sprite[index].z + tileHeight(sprite[index].picnum) / 2 + tileTopOffset(sprite[index].picnum);
                                break;
                            case 5:
                            case 6:
                                if (!sectRangeIsFine(sprite[index].sectnum)) sprite[nSprite].z = top;
                                else sprite[nSprite].z = (pXSource->data3 == 5) ? sector[sprite[nSprite].sectnum].floorz : sector[sprite[nSprite].sectnum].ceilingz;
                                break;
                        }
                        
                        if (nSprite >= 0) {
                            
                            int nXSprite = dbInsertXSprite(nSprite);
                            seqSpawn(pXSource->data2, 3, nXSprite, -1);
                            if (pSource->flags & kModernTypeFlag1) {

                                sprite[nSprite].pal = pSource->pal;
                                sprite[nSprite].shade = pSource->shade;
                                sprite[nSprite].xrepeat = pSource->xrepeat;
                                sprite[nSprite].yrepeat = pSource->yrepeat;
                                sprite[nSprite].xoffset = pSource->xoffset;
                                sprite[nSprite].yoffset = pSource->yoffset;

                            }

                            if (pSource->flags & kModernTypeFlag2) {
                                
                                sprite[nSprite].cstat |= pSource->cstat;

                            }

                            // should be: the more is seqs, the shorter is timer
                            evPost(nSprite, OBJ_SPRITE, 1000, kCallbackRemove);
                        }
                    } else {

                seqSpawn(pXSource->data2, 3, sprite[index].extra, -1);

                    }
                if (pXSource->data4 > 0)
                    sfxPlay3DSound(&sprite[index], pXSource->data4, -1, 0);
            }
            return;
    }
}

int condSerialize(int objType, int objIndex) {
    switch (objType) {
        case OBJ_SECTOR: return kCondSerialSector + objIndex;
        case OBJ_WALL:   return kCondSerialWall + objIndex;
        case OBJ_SPRITE: return kCondSerialSprite + objIndex;
    }
    I_Error("Unknown object type %d, index %d", objType, objIndex);
    return -1;
}

void condUnserialize(int serial, int* objType, int* objIndex) {
    if (serial >= kCondSerialSector && serial < kCondSerialWall) {
        
        *objIndex = serial - kCondSerialSector;
        *objType = OBJ_SECTOR; 

    } else if (serial >= kCondSerialWall && serial < kCondSerialSprite) {
        
        *objIndex = serial - kCondSerialWall;
        *objType = OBJ_WALL; 

    } else if (serial >= kCondSerialSprite && serial < kCondSerialMax) {
        
        *objIndex = serial - kCondSerialSprite;
        *objType = OBJ_SPRITE; 

    } else {
        
        I_Error("%d is not condition serial!", serial);

    }
}

bool condPush(XSPRITE* pXSprite, int objType, int objIndex) {
    pXSprite->targetX = condSerialize(objType, objIndex);
    return true;
}

bool condRestore(XSPRITE* pXSprite) {
    pXSprite->targetX = pXSprite->targetY;
    return true;
}

// normal comparison
bool condCmp(int val, int arg1, int arg2, int comOp) {
    if (comOp & 0x2000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val > arg1) : (val >= arg1); // blue sprite
    else if (comOp & 0x4000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val < arg1) : (val <= arg1); // green sprite
    else if (comOp & CSTAT_SPRITE_BLOCK) {
        if (arg1 > arg2) I_Error("Value of argument #1 (%d) must be less than value of argument #2 (%d)", arg1, arg2);
        return (val >= arg1 && val <= arg2);
    }
    else return (val == arg1);
}

// no extra comparison (val always = 0)?
bool condCmpne(int arg1, int arg2, int comOp) {

    if (comOp & 0x2000) return (comOp & CSTAT_SPRITE_BLOCK) ? (0 > arg1) : (0 >= arg1); // blue sprite
    else if (comOp & 0x4000) return (comOp & CSTAT_SPRITE_BLOCK) ? (0 < arg1) : (0 <= arg1); // green sprite
    else if (comOp & CSTAT_SPRITE_BLOCK) {
        if (arg1 > arg2) I_Error("Value of argument #1 (%d) must be less than value of argument #2 (%d)", arg1, arg2);
        return (0 >= arg1 && 0 <= arg2);
    }
    else return (0 == arg1);

}

// bool comparison
bool condCmpb(int val, int arg1, int arg2, int comOp) {
    
    arg1 = ClipRange(arg1, 0, 1); arg2 = ClipRange(arg2, 0, 1);
    if (comOp & 0x2000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val > 0) : (val >= 0); // blue sprite
    else if (comOp & 0x4000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val < arg1) : (val <= arg1); // green sprite
    else if (comOp & CSTAT_SPRITE_BLOCK) {
        if (arg1 > arg2) I_Error("Value of argument #1 (%d) must be less than value of argument #2 (%d)", arg1, arg2);
        return (val >= arg1 && val <= arg2);
    }
    else return (val == arg1);

}

void condError(XSPRITE* pXCond, const char* pzFormat, ...) {
   
    char buffer[256]; char buffer2[512];
    sprintf(buffer, "\nCONDITION RX: %d, TX: %d, SPRITE: #%d RETURNS:\n----------\n\n", pXCond->rxID, pXCond->txID, pXCond->reference);
    va_list args;
    va_start(args, pzFormat);
    vsprintf(buffer2, pzFormat, args);
    I_Error("%s%s", buffer, buffer2);
}


bool condCheckMixed(XSPRITE* pXCond, EVENT event, int cmpOp, bool PUSH) {
    
    //int var = -1;
    int cond = pXCond->data1 - kCondMixedBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; int arg3 = pXCond->data4;
    
    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    switch (cond) {
        case 0:  return (objType == OBJ_SECTOR && sectRangeIsFine(objIndex)); // is a sector?
        case 5:  return (objType == OBJ_WALL && wallRangeIsFine(objIndex));   // is a wall?
        case 10: return (objType == OBJ_SPRITE && spriRangeIsFine(objIndex)); // is a sprite?
        case 15: // x-index is fine?
            switch (objType) {
                case OBJ_WALL: return xwallRangeIsFine(wall[objIndex].extra);
                case OBJ_SPRITE: return xspriRangeIsFine(sprite[objIndex].extra);
                case OBJ_SECTOR: return xsectRangeIsFine(sector[objIndex].extra);
            }
            break;
        case 20: // type in a range?
            switch (objType) {
                case OBJ_WALL:
                    return condCmp(wall[objIndex].type, arg1, arg2, cmpOp);
                case OBJ_SPRITE:
                    return condCmp((sprite[objIndex].type != kThingBloodChunks) ? sprite[objIndex].type : sprite[objIndex].inittype, arg1, arg2, cmpOp);
                case OBJ_SECTOR:
                    return condCmp(sector[objIndex].type, arg1, arg2, cmpOp);
            }
            break;
        case 24:
        case 25: case 26: case 27:
        case 28: case 29: case 30:
        case 31: case 32: case 33:
            switch (objType) {
                case OBJ_WALL: {
                    walltype* pObj = &wall[objIndex];
                    switch (cond) {
                        //case 23: // compare picnum size in %?
                            
                            //return condCmp(tilesiz[pObj->picnum].x, arg1, arg2, cmpOp);
                        case 24: return condCmp(surfType[wall[objIndex].picnum], arg1, arg2, cmpOp);
                        case 25: return condCmp(pObj->picnum, arg1, arg2, cmpOp);
                        case 26: return condCmp(pObj->pal, arg1, arg2, cmpOp);
                        case 27: return condCmp(pObj->shade, arg1, arg2, cmpOp);
                        case 28: return (pObj->cstat & arg1);
                        case 29: return (pObj->hitag & arg1);
                        case 30: return condCmp(pObj->xrepeat, arg1, arg2, cmpOp);
                        case 31: return condCmp(pObj->xpan(), arg1, arg2, cmpOp);
                        case 32: return condCmp(pObj->yrepeat, arg1, arg2, cmpOp);
                        case 33: return condCmp(pObj->ypan(), arg1, arg2, cmpOp);
                    }
                    break;
                }
                case OBJ_SPRITE: {
                    spritetype* pObj = &sprite[objIndex];
                    switch (cond) {
                        case 24: return condCmp(surfType[sprite[objIndex].picnum], arg1, arg2, cmpOp);
                        case 25: return condCmp(pObj->picnum, arg1, arg2, cmpOp);
                        case 26: return condCmp(pObj->pal, arg1, arg2, cmpOp);
                        case 27: return condCmp(pObj->shade, arg1, arg2, cmpOp);
                        case 28: return (pObj->cstat & arg1);
                        case 29: return (pObj->hitag & arg1);
                        case 30: return condCmp(pObj->xrepeat, arg1, arg2, cmpOp);
                        case 31: return condCmp(pObj->xoffset, arg1, arg2, cmpOp);
                        case 32: return condCmp(pObj->yrepeat, arg1, arg2, cmpOp);
                        case 33: return condCmp(pObj->yoffset, arg1, arg2, cmpOp);
                    }
                    break;
                }
                case OBJ_SECTOR: {
                    sectortype* pObj = &sector[objIndex];
                    switch (cond) {
                        case 24:
                            switch (arg3) {
                                default: return (condCmp(surfType[sector[objIndex].floorpicnum], arg1, arg2, cmpOp) || condCmp(surfType[sector[objIndex].ceilingpicnum], arg1, arg2, cmpOp));
                                case 1: return condCmp(surfType[sector[objIndex].floorpicnum], arg1, arg2, cmpOp);
                                case 2: return condCmp(surfType[sector[objIndex].ceilingpicnum], arg1, arg2, cmpOp);
                            }
                            break;
                        case 25:
                            switch (arg3) {
                                default: return (condCmp(pObj->floorpicnum, arg1, arg2, cmpOp) || condCmp(pObj->ceilingpicnum, arg1, arg2, cmpOp));
                                case 1:  return condCmp(pObj->floorpicnum, arg1, arg2, cmpOp);
                                case 2:  return condCmp(pObj->ceilingpicnum, arg1, arg2, cmpOp);
                            }
                            break;
                        case 26: 
                            switch (arg3) {
                                default: return (condCmp(pObj->floorpal, arg1, arg2, cmpOp) || condCmp(pObj->ceilingpal, arg1, arg2, cmpOp));
                                case 1:  return condCmp(pObj->floorpal, arg1, arg2, cmpOp);
                                case 2:  return condCmp(pObj->ceilingpal, arg1, arg2, cmpOp);
                            }
                            break;
                        case 27:
                            switch (arg3) {
                                default: return (condCmp(pObj->floorshade, arg1, arg2, cmpOp) || condCmp(pObj->ceilingshade, arg1, arg2, cmpOp));
                                case 1:  return condCmp(pObj->floorshade, arg1, arg2, cmpOp);
                                case 2:  return condCmp(pObj->ceilingshade, arg1, arg2, cmpOp);
                            }
                            break;
                        case 28:
                            switch (arg3) {
                                default: return ((pObj->floorstat & arg1) || (pObj->ceilingshade & arg1));
                                case 1:  return (pObj->floorstat & arg1);
                                case 2:  return (pObj->ceilingshade & arg1);
                            }
                            break;
                        case 29: return (pObj->hitag & arg1);
                        case 30: return condCmp(pObj->floorxpan(), arg1, arg2, cmpOp);
                        case 31: return condCmp(pObj->ceilingxpan(), arg1, arg2, cmpOp);
                        case 32: return condCmp(pObj->floorypan(), arg1, arg2, cmpOp);
                        case 33: return condCmp(pObj->ceilingypan(), arg1, arg2, cmpOp);
                    }
                    break;
                }
            }
            break;
        case 41:  case 42:  case 43:
        case 44:  case 50:  case 51:
        case 52:  case 53:  case 54:
        case 55:  case 56:  case 57:
        case 58:  case 59:  case 70:
        case 71:
            switch (objType) {
                case OBJ_WALL: {
                    XWALL* pXObj = (xwallRangeIsFine(wall[objIndex].extra)) ? &xwall[wall[objIndex].extra] : NULL;
                    if (!pXObj) return condCmpne(arg1, arg2, cmpOp);
                    switch (cond) {
                        case 41: return condCmp(pXObj->data, arg1, arg2, cmpOp);
                        case 50: return condCmp(pXObj->rxID, arg1, arg2, cmpOp);
                        case 51: return condCmp(pXObj->txID, arg1, arg2, cmpOp);
                        case 52: return condCmpb(pXObj->locked, arg1, arg2, cmpOp);
                        case 53: return condCmpb(pXObj->triggerOn, arg1, arg2, cmpOp);
                        case 54: return condCmpb(pXObj->triggerOff, arg1, arg2, cmpOp);
                        case 55: return condCmpb(pXObj->triggerOnce, arg1, arg2, cmpOp);
                        case 56: return condCmpb(pXObj->isTriggered, arg1, arg2, cmpOp);
                        case 57: return condCmpb(pXObj->state, arg1, arg2, cmpOp);
                        case 58: return condCmp((kPercFull * pXObj->busy) / 65536, arg1, arg2, cmpOp);
                        case 59: return condCmpb(pXObj->dudeLockout, arg1, arg2, cmpOp);
                        case 70:
                            switch (arg3) {
                                default: return (condCmp(seqGetID(0, wall[objIndex].extra), arg1, arg2, cmpOp) || condCmp(seqGetID(4, wall[objIndex].extra), arg1, arg2, cmpOp));
                                case 1:  return condCmp(seqGetID(0, wall[objIndex].extra), arg1, arg2, cmpOp);
                                case 2:  return condCmp(seqGetID(4, wall[objIndex].extra), arg1, arg2, cmpOp);
                            }
                            break;
                        case 71:
                            switch (arg3) {
                                default: return (condCmp(seqGetStatus(0, wall[objIndex].extra), arg1, arg2, cmpOp) || condCmp(seqGetStatus(4, wall[objIndex].extra), arg1, arg2, cmpOp));
                                case 1:  return condCmp(seqGetStatus(0, wall[objIndex].extra), arg1, arg2, cmpOp);
                                case 2:  return condCmp(seqGetStatus(4, wall[objIndex].extra), arg1, arg2, cmpOp);
                            }
                            break;
                    }
                    break;
                }
                case OBJ_SPRITE: {
                    XSPRITE* pXObj = (xspriRangeIsFine(sprite[objIndex].extra)) ? &xsprite[sprite[objIndex].extra] : NULL;
                    if (!pXObj) return condCmpne(arg1, arg2, cmpOp);
                    switch (cond) {
                        case 41: case 42:
                        case 43: case 44:
                            return condCmp(getDataFieldOfObject(OBJ_SPRITE, objIndex, 1 + cond - 41), arg1, arg2, cmpOp);
                        case 50: return condCmp(pXObj->rxID, arg1, arg2, cmpOp);
                        case 51: return condCmp(pXObj->txID, arg1, arg2, cmpOp);
                        case 52: return condCmpb(pXObj->locked, arg1, arg2, cmpOp);
                        case 53: return condCmpb(pXObj->triggerOn, arg1, arg2, cmpOp);
                        case 54: return condCmpb(pXObj->triggerOff, arg1, arg2, cmpOp);
                        case 55: return condCmpb(pXObj->triggerOnce, arg1, arg2, cmpOp);
                        case 56: return condCmpb(pXObj->isTriggered, arg1, arg2, cmpOp);
                        case 57: return condCmpb(pXObj->state, arg1, arg2, cmpOp);
                        case 58: return condCmp((kPercFull * pXObj->busy) / 65536, arg1, arg2, cmpOp);
                        case 59: return condCmpb(pXObj->DudeLockout, arg1, arg2, cmpOp);
                        case 70: return condCmp(seqGetID(3, sprite[objIndex].extra), arg1, arg2, cmpOp);
                        case 71: return condCmp(seqGetStatus(3, sprite[objIndex].extra), arg1, arg2, cmpOp);
                    }
                    break;
                }
                case OBJ_SECTOR: {
                    XSECTOR* pXObj = (xsectRangeIsFine(sector[objIndex].extra)) ? &xsector[sector[objIndex].extra] : NULL;
                    if (!pXObj) return condCmpne(arg1, arg2, cmpOp);
                    switch (cond) {
                        case 41: return condCmp(pXObj->data, arg1, arg2, cmpOp);
                        case 50: return condCmp(pXObj->rxID, arg1, arg2, cmpOp);
                        case 51: return condCmp(pXObj->txID, arg1, arg2, cmpOp);
                        case 52: return condCmpb(pXObj->locked, arg1, arg2, cmpOp);
                        case 53: return condCmpb(pXObj->triggerOn, arg1, arg2, cmpOp);
                        case 54: return condCmpb(pXObj->triggerOff, arg1, arg2, cmpOp);
                        case 55: return condCmpb(pXObj->triggerOnce, arg1, arg2, cmpOp);
                        case 56: return condCmpb(pXObj->isTriggered, arg1, arg2, cmpOp);
                        case 57: return condCmpb(pXObj->state, arg1, arg2, cmpOp);
                        case 58: return condCmp((kPercFull * pXObj->busy) / 65536, arg1, arg2, cmpOp);
                        case 59: return condCmpb(pXObj->dudeLockout, arg1, arg2, cmpOp);
                        case 70:
                            switch (arg3) {
                                default: return (condCmp(seqGetID(1, wall[objIndex].extra), arg1, arg2, cmpOp) || condCmp(seqGetID(2, wall[objIndex].extra), arg1, arg2, cmpOp));
                                case 1:  return condCmp(seqGetID(1, wall[objIndex].extra), arg1, arg2, cmpOp);
                                case 2:  return condCmp(seqGetID(2, wall[objIndex].extra), arg1, arg2, cmpOp);
                            }
                            break;
                        case 71:
                            switch (arg3) {
                                default: return (condCmp(seqGetStatus(1, wall[objIndex].extra), arg1, arg2, cmpOp) || condCmp(seqGetStatus(2, wall[objIndex].extra), arg1, arg2, cmpOp));
                                case 1:  return condCmp(seqGetStatus(1, wall[objIndex].extra), arg1, arg2, cmpOp);
                                case 2:  return condCmp(seqGetStatus(2, wall[objIndex].extra), arg1, arg2, cmpOp);
                            }
                            break;
                    }
                    break;
                }
            }
            break;
        case 99: return condCmp(event.cmd, arg1, arg2, cmpOp);  // this codition received specified command?
    }

    condError(pXCond, "Mixed: Unexpected condition id (%d)!", cond);
    return false;
}

bool condCheckSector(XSPRITE* pXCond, int cmpOp, bool PUSH) {

    int var = -1;
    int cond = pXCond->data1 - kCondSectorBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; //int arg3 = pXCond->data4;
    
    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    if (objType != OBJ_SECTOR || !sectRangeIsFine(objIndex))
        condError(pXCond, "Sector conditions:\nObject #%d (objType: %d) is not a sector!", objIndex, objType);

    sectortype* pSect = &sector[objIndex];
    XSECTOR* pXSect = (xsectRangeIsFine(pSect->extra)) ? &xsector[pSect->extra] : NULL;

    if (cond < (kCondRange >> 1)) {
        switch (cond) {
        default: break;
        case 0: return condCmp(pSect->visibility, arg1, arg2, cmpOp);
        case 5: return condCmp(pSect->floorheinum, arg1, arg2, cmpOp);
        case 6: return condCmp(pSect->ceilingheinum, arg1, arg2, cmpOp);
        case 10: // required sprite type is in current sector?
            int nSprite;
            SectIterator it(objIndex);
            while ((nSprite = it.NextIndex()) >= 0)
            {
                if (!condCmp(sprite[var].type, arg1, arg2, cmpOp)) continue;
                else if (PUSH) condPush(pXCond, OBJ_SPRITE, var);
                return true;
            }
            return false;
        }
    } else if (pXSect) {
        switch (cond) {
            default: break;
            case 50: return pXSect->Underwater;
            case 51: return condCmp(pXSect->Depth, arg1, arg2, cmpOp);
            case 55: // compare floor height (in %)
            case 56: { // compare ceil height (in %)
                int h = 0; int curH = 0;
                switch (pSect->type) {
                case kSectorZMotion:
                case kSectorRotate:
                case kSectorSlide:
                    if (cond == 60) {
                        h = ClipLow(abs(pXSect->onFloorZ - pXSect->offFloorZ), 1);
                        curH = abs(pSect->floorz - pXSect->offFloorZ);
                    } else {
                        h = ClipLow(abs(pXSect->onCeilZ - pXSect->offCeilZ), 1);
                        curH = abs(pSect->ceilingz - pXSect->offCeilZ);
                    }
                    return condCmp((kPercFull * curH) / h, arg1, arg2, cmpOp);
                default:
                    condError(pXCond, "Sector conditions:\nUsupported sector type %d", pSect->type);
                    return false;
                }
            }
            case 57: // this sector in movement?
                return !pXSect->unused1;
        }
    } else {
        switch (cond) {
            default: return false;
            case 55:
            case 56:
                return condCmpne(arg1, arg2, cmpOp);
        }
    }
    
    condError(pXCond, "Sector conditions: Unexpected condition id (%d)!", cond);
    return false;
}

bool condCheckWall(XSPRITE* pXCond, int cmpOp, bool PUSH) {

    int var = -1;
    int cond = pXCond->data1 - kCondWallBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; //int arg3 = pXCond->data4;
    
    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    if (objType != OBJ_WALL || !wallRangeIsFine(objIndex))
        condError(pXCond, "Wall conditions:\nObject #%d (objType: %d) is not a wall!", objIndex, objType);
        
    walltype* pWall = &wall[objIndex];
    //XWALL* pXWall = (xwallRangeIsFine(pWall->extra)) ? &xwall[pWall->extra] : NULL;
    
    if (cond < (kCondRange >> 1)) {
        switch (cond) {
            default: break;
            case 0:
                return condCmp(pWall->overpicnum, arg1, arg2, cmpOp);
            case 5:
                if (!sectRangeIsFine((var = sectorofwall(objIndex)))) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, var);
                return true;
            case 10: // this wall is a mirror?                          // must be as constants here
                return (pWall->type != kWallStack && condCmp(pWall->picnum, 4080, (4080 + 16) - 1, 0));
            case 15:
                if (!sectRangeIsFine(pWall->nextsector)) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, pWall->nextsector);
                return true;
            case 20:
                if (!wallRangeIsFine(pWall->nextwall)) return false;
                else if (PUSH) condPush(pXCond, OBJ_WALL, pWall->nextwall);
                return true;
            case 25: // next wall belongs to sector?
                if (!sectRangeIsFine(var = sectorofwall(pWall->nextwall))) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, var);
                return true;
        }
    }

    condError(pXCond, "Wall conditions: Unexpected condition id (%d)!", cond);
    return false;
}

bool condCheckPlayer(XSPRITE* pXCond, int cmpOp, bool PUSH) {

    int var = -1; PLAYER* pPlayer = NULL;
    int cond = pXCond->data1 - kCondPlayerBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; int arg3 = pXCond->data4;

    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    if (objType == OBJ_SPRITE) {
        for (int i = 0; i < kMaxPlayers; i++) {
            if (objIndex != gPlayer[i].nSprite) continue;
            pPlayer = &gPlayer[i];
            break;
        }
    }

    spritetype* pSpr = NULL;
    if (spriRangeIsFine(objIndex) && pPlayer) pSpr = pPlayer->pSprite;
    else condError(pXCond, "\nPlayer conditions:\nObject #%d (objType: %d) is not a player!", objIndex, objType);

    switch (cond) {
        case 0: // check if this player is connected
            if (!condCmp(pPlayer->nPlayer + 1, arg1, arg2, cmpOp) || !spriRangeIsFine(pPlayer->nSprite)) return false;
            else if (PUSH) condPush(pXCond, OBJ_SPRITE, pPlayer->nSprite);
            return (pPlayer->nPlayer >= 0);
        case 1: return condCmp((gGameOptions.nGameType != 3) ? 0 : pPlayer->teamId + 1, arg1, arg2, cmpOp); // compare team
        case 2: return (arg1 > 0 && arg1 < 8 && pPlayer->hasKey[arg1 - 1]);
        case 3: return (arg1 > 0 && arg1 < 15 && pPlayer->hasWeapon[arg1 - 1]);
        case 4: return condCmp(pPlayer->curWeapon, arg1, arg2, cmpOp);
        case 5: return (arg1 > 0 && arg1 < 6 && condCmp(pPlayer->packSlots[arg1 - 1].curAmount, arg2, arg3, cmpOp));
        case 6: return (arg1 > 0 && arg1 < 6 && pPlayer->packSlots[arg1 - 1].isActive);
        case 7: return condCmp(pPlayer->packItemId + 1, arg1, arg2, cmpOp);
        case 8: // check for powerup amount in %
            if (arg3 > 0 && arg3 < 30) var = (12 + arg3) - 1; // allowable powerups
            else condError(pXCond, "Unexpected powerup #%d", arg3);
            return condCmp((kPercFull * pPlayer->pwUpTime[var]) / gPowerUpInfo[var].bonusTime, arg1, arg2, cmpOp);
        case 9:
            if (!spriRangeIsFine(pPlayer->fraggerId)) return false;
            else if (PUSH) condPush(pXCond, OBJ_SPRITE, pPlayer->fraggerId);
            return true;
        case 10: // check keys pressed
            switch (arg1) {
            case 1:  return (pPlayer->input.fvel > 0);            // forward
            case 2:  return (pPlayer->input.fvel < 0);            // backward
            case 3:  return (pPlayer->input.svel > 0);             // left
            case 4:  return (pPlayer->input.svel < 0);             // right
            case 5:  return !!(pPlayer->input.actions & SB_JUMP);       // jump
            case 6:  return !!(pPlayer->input.actions & SB_CROUCH);     // crouch
            case 7:  return !!(pPlayer->input.actions & SB_FIRE);      // normal fire weapon
            case 8:  return !!(pPlayer->input.actions & SB_ALTFIRE);     // alt fire weapon
            default:
                condError(pXCond, "Player conditions:\nSpecify a correct key!");
                break;
            }
            return false;
        case 11: return (pPlayer->isRunning);
        case 12: return (pPlayer->fallScream); // falling in abyss?
        case 13: return condCmp(pPlayer->lifeMode + 1, arg1, arg2, cmpOp);
        case 14: return condCmp(pPlayer->posture + 1, arg1, arg2, cmpOp);
        case 46: return condCmp(pPlayer->sceneQav, arg1, arg2, cmpOp);
        case 47: return (pPlayer->godMode || powerupCheck(pPlayer, kPwUpDeathMask));
        case 48: return isShrinked(pSpr);
        case 49: return isGrown(pSpr);
    }

    condError(pXCond, "Player conditions:\nUnexpected condition #%d!", cond);
    return false;
}

bool condCheckDude(XSPRITE* pXCond, int cmpOp, bool PUSH) {

    int var = -1; //PLAYER* pPlayer = NULL;
    int cond = pXCond->data1 - kCondDudeBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; //int arg3 = pXCond->data4;
    
    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    if (objType != OBJ_SPRITE || !spriRangeIsFine(objIndex))
        condError(pXCond, "Dude conditions:\nObject #%d (objType: %d) is not a dude!", objIndex, objType);
    
    spritetype* pSpr = &sprite[objIndex]; int nType = pSpr->type;
        
    if (nType == kThingBloodChunks || !xspriRangeIsFine(pSpr->extra)) {
            
        nType = pSpr->inittype;
        if (nType >= kDudeBase && nType <= kDudeMax && (nType < kDudePlayer1 || nType > kDudePlayer8)) return false;
        else condError(pXCond, "Dude conditions:\nObject #%d (objType: %d) is not an enemy!", objIndex, objType);
           
    } else if (IsDudeSprite(pSpr)) {
            
        XSPRITE* pXSpr = &xsprite[pSpr->extra];

        if (pSpr->flags & kHitagRespawn || pSpr->statnum == kStatRespawn) return false;
        else if (IsPlayerSprite(pSpr)) condError(pXCond, "Dude conditions:\nObject #%d (objType: %d) is not an enemy!", objIndex, objType);
        
        switch (cond) {
            default: break;
            case 0: // dude have any targets?
                if (!spriRangeIsFine(pXSpr->target)) return false;
                else if (!IsDudeSprite(&sprite[pXSpr->target]) && sprite[pXSpr->target].type != kMarkerPath) return false;
                else if (PUSH) condPush(pXCond, OBJ_SPRITE, pXSpr->target);
                return true;
            case 1: return aiFightDudeIsAffected(pXSpr); // dude affected by ai fight?
            case 2: // distance to the target in a range?
            case 3: // is the target visible?
            case 4: // is the target visible with periphery?
            {
                DUDEINFO* pInfo = getDudeInfo(pSpr->type);
                int eyeAboveZ = pInfo->eyeHeight * pSpr->yrepeat << 2;
                if (!spriRangeIsFine(pXSpr->target))
                    condError(pXCond, "Dude #%d have no target!", objIndex);

                spritetype* pTrgt = &sprite[pXSpr->target];
                int dx = pTrgt->x - pSpr->x; int dy = pTrgt->y - pSpr->y;

                switch (cond) {
                    case 2: 
                        var = condCmp(approxDist(dx, dy), arg1 * 512, arg2 * 512, cmpOp);
                        break;
                    case 3:
                    case 4:
                        var = cansee(pSpr->x, pSpr->y, pSpr->z, pSpr->sectnum, pTrgt->x, pTrgt->y, pTrgt->z - eyeAboveZ, pTrgt->sectnum);
                        if (cond == 4 && var > 0) {
                            var = ((1024 + getangle(dx, dy) - pSpr->ang) & 2047) - 1024;
                            var = (abs(var) < ((arg1 <= 0) ? pInfo->periphery : ClipHigh(arg1, 2048)));
                        }
                        break;
                }

                if (var <= 0) return false;
                else if (PUSH) condPush(pXCond, OBJ_SPRITE, pXSpr->target);
                return true;

            }
            case 5: return pXSpr->dudeFlag4;
            case 6: return pXSpr->dudeDeaf;
            case 7: return pXSpr->dudeGuard;
            case 8: return pXSpr->dudeAmbush;
            case 9: return (pXSpr->unused1 & kDudeFlagStealth);
            case 10: // check if the marker is busy with another dude
            case 11: // check if the marker is reached
                if (!pXSpr->dudeFlag4 || !spriRangeIsFine(pXSpr->target) || sprite[pXSpr->target].type != kMarkerPath) return false;
                switch (cond) {
                    case 9:
                        var = aiPatrolMarkerBusy(pSpr->index, pXSpr->target);
                        if (!spriRangeIsFine(var)) return false;
                        else if (PUSH) condPush(pXCond, OBJ_SPRITE, pXSpr->target);
                        break;
                    case 10:
                        if (!aiPatrolMarkerReached(pSpr, pXSpr)) return false;
                        else if (PUSH) condPush(pXCond, OBJ_SPRITE, pXSpr->target);
                        break;
                }
                return true;
            case 12: // compare spot progress value in %
                if (!pXSpr->dudeFlag4 || !spriRangeIsFine(pXSpr->target) || sprite[pXSpr->target].type != kMarkerPath) var = 0;
                else if (pXSpr->data3 < 0 || pXSpr->data3 > kMaxPatrolSpotValue) var = 0;
                else var = (kPercFull * pXSpr->data3) / kMaxPatrolSpotValue;
                return condCmp(var, arg1, arg2, cmpOp);
            case 15: return getDudeInfo(pSpr->type)->lockOut; // dude allowed to interact with objects?
            case 20: // kDudeModernCustom conditions
            case 21:
            case 22:
                switch (pSpr->type) {
                case kDudeModernCustom:
                case kDudeModernCustomBurning:
                    switch (cond) {
                        case 20: // life leech is thrown?
                            var = genDudeExtra(pSpr)->nLifeLeech;
                            if (!spriRangeIsFine(var)) return false;
                            else if (PUSH) condPush(pXCond, OBJ_SPRITE, var);
                            return true;
                        case 21: // life leech is destroyed?
                            var = genDudeExtra(pSpr)->nLifeLeech;
                            if (!spriRangeIsFine(var) && pSpr->owner == kMaxSprites - 1) return true;
                            else if (PUSH) condPush(pXCond, OBJ_SPRITE, var);
                            return false;
                        case 22: // are required amount of dudes is summoned?
                            return condCmp(gGenDudeExtra[pSpr->index].slaveCount, arg1, arg2, cmpOp);
                            break;
                    }
                    fallthrough__;
                default:
                    condError(pXCond, "Dude #%d is not a Custom Dude!", objIndex);
                    return false;
                }
        }

    }

    condError(pXCond, "Dude conditions:\nUnexpected condition #%d!", cond);
    return false;
}

bool condCheckSprite(XSPRITE* pXCond, int cmpOp, bool PUSH) {

    auto actor = &bloodActors[pXCond->reference];
    int var = -1, var2 = -1, var3 = -1; PLAYER* pPlayer = NULL; bool retn = false;
    int cond = pXCond->data1 - kCondSpriteBase; int arg1 = pXCond->data2;
    int arg2 = pXCond->data3; int arg3 = pXCond->data4;
    
    int objType = -1; int objIndex = -1;
    condUnserialize(pXCond->targetX, &objType, &objIndex);

    if (objType != OBJ_SPRITE || !spriRangeIsFine(objIndex))
        condError(pXCond, "Sprite condition %d:\nObject #%d (objType: %d) is not a sprite!", cond, objIndex, objType);

    spritetype* pSpr = &sprite[objIndex];
    XSPRITE* pXSpr = (xspriRangeIsFine(pSpr->extra)) ? &xsprite[pSpr->extra] : NULL;
    DBloodActor* spractor = &bloodActors[pXSpr->reference];
    
    if (cond < (kCondRange >> 1)) {
        switch (cond) {
            default: break;
            case 0: return condCmp((pSpr->ang & 2047), arg1, arg2, cmpOp);
            case 5: return condCmp(pSpr->statnum, arg1, arg2, cmpOp);
            case 6: return ((pSpr->flags & kHitagRespawn) || pSpr->statnum == kStatRespawn);
            case 7: return condCmp(spriteGetSlope(pSpr->index), arg1, arg2, cmpOp);
            case 10: return condCmp(pSpr->clipdist, arg1, arg2, cmpOp);
            case 15:
                if (!spriRangeIsFine(pSpr->owner)) return false;
                else if (PUSH) condPush(pXCond, OBJ_SPRITE, pSpr->owner);
                return true;
            case 20: // stays in a sector?
                if (!sectRangeIsFine(pSpr->sectnum)) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, pSpr->sectnum);
                return true;
            case 25:
                switch (arg1) {
                    case 0: return (xvel[pSpr->index] || yvel[pSpr->index] || zvel[pSpr->index]);
                    case 1: return (xvel[pSpr->index]);
                    case 2: return (yvel[pSpr->index]);
                    case 3: return (zvel[pSpr->index]);
                }
                break;
            case 30:
                if (!spriteIsUnderwater(pSpr) && !spriteIsUnderwater(pSpr, true)) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, pSpr->sectnum);
                return true;
            case 35: // hitscan: ceil?
            case 36: // hitscan: floor?
            case 37: // hitscan: wall?
            case 38: // hitscan: sprite?
                switch (arg1) {
                    case  0: arg1 = CLIPMASK0 | CLIPMASK1; break;
                    case  1: arg1 = CLIPMASK0; break;
                    case  2: arg1 = CLIPMASK1; break;
                }

                var3 = spriteGetSlope(pSpr->index);
                var2 = pSpr->cstat;  pSpr->cstat = 0;
                if ((pPlayer = getPlayerById(pSpr->type)) != NULL)
                    var = HitScan(pSpr, pPlayer->zWeapon - pSpr->z, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz, arg1, arg3 << 1);
                else if (IsDudeSprite(pSpr))
                    var = HitScan(pSpr, pSpr->z, Cos(pSpr->ang) >> 16, Sin(pSpr->ang) >> 16, (!xspriRangeIsFine(pSpr->extra)) ? 0 : spractor->dudeSlope, arg1, arg3 << 1);
                else if (var2 & CSTAT_SPRITE_ALIGNMENT_FLOOR) {
                    
                    if (var3 == 0) {
                        if (var2 & 0x0008) var = 0x10000;
                        else var = -0x10000;
                    }
                    else if (var3 > 0) var3 = ClipLow(34816 - abs(var3), 0);
                    else var3 = -ClipLow(34816 - abs(var3), 0);
                    
                    var = HitScan(pSpr, pSpr->z, Cos(pSpr->ang) >> 16, Sin(pSpr->ang) >> 16, var3, arg1, arg3 << 1);

                } else {
                    
                    var = HitScan(pSpr, pSpr->z, CosScale16(pSpr->ang), SinScale16(pSpr->ang), 0, arg1, arg3 << 1);

                }
                pSpr->cstat = var2;

                if (var < 0) return retn;
                    switch (cond) {
                        case 35: retn = (var == 1); break;
                        case 36: retn = (var == 2); break;
                        case 37: retn = (var == 0 || var == 4); break;
                        case 38: retn = (var == 3); break;
                    }

                if (!PUSH) return retn;
                        switch (var) {
                    case 0: case 4: condPush(pXCond, OBJ_WALL, gHitInfo.hitwall);       break;
                    case 1: case 2: condPush(pXCond, OBJ_SECTOR, gHitInfo.hitsect);     break;
                    case 3:         condPush(pXCond, OBJ_SPRITE, gHitInfo.hitsprite);   break;
                        }
                return retn;
            case 45: // this sprite is a target of some dude?
                int nSprite;
                StatIterator it(kStatDude);
                while ((nSprite = it.NextIndex()) >= 0)
                {
                    if (pSpr->index == nSprite) continue;

                    spritetype* pDude = &sprite[nSprite];
                    if (IsDudeSprite(pDude) && xspriRangeIsFine(pDude->extra)) {
                        XSPRITE* pXDude = &xsprite[pDude->extra];
                        if (pXDude->health <= 0 || pXDude->target != pSpr->index) continue;
                        else if (PUSH) condPush(pXCond, OBJ_SPRITE, nSprite);
                        return true;
                    }
                }
                return false;
        }
    } else if (pXSpr) {
        switch (cond) {
            default: break;
            case 50: // compare hp (in %)
                if (IsDudeSprite(pSpr)) var = (pXSpr->sysData2 > 0) ? ClipRange(pXSpr->sysData2 << 4, 1, 65535) : getDudeInfo(pSpr->type)->startHealth << 4;
                else if (pSpr->type == kThingBloodChunks) return condCmpne(arg1, arg2, cmpOp);
                else if (pSpr->type >= kThingBase && pSpr->type < kThingMax) var = thingInfo[pSpr->type - kThingBase].startHealth << 4;
                return condCmp((kPercFull * pXSpr->health) / ClipLow(var, 1), arg1, arg2, cmpOp);
            case 55: // touching ceil of sector?
                if ((gSpriteHit[pSpr->extra].ceilhit & 0xc000) != 0x4000) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, gSpriteHit[pSpr->extra].ceilhit & 0x3fff);
                return true;
            case 56: // touching floor of sector?
                if ((gSpriteHit[pSpr->extra].florhit & 0xc000) != 0x4000) return false;
                else if (PUSH) condPush(pXCond, OBJ_SECTOR, gSpriteHit[pSpr->extra].florhit & 0x3fff);
                return true;
            case 57: // touching walls of sector?
                if ((gSpriteHit[pSpr->extra].hit & 0xc000) != 0x8000) return false;
                else if (PUSH) condPush(pXCond, OBJ_WALL, gSpriteHit[pSpr->extra].hit & 0x3fff);
                return true;
            case 58: // touching another sprite?
                switch (arg3) {
                    case 0:
                    case 1:
                        if ((gSpriteHit[pSpr->extra].florhit & 0xc000) == 0xc000) var = gSpriteHit[pSpr->extra].florhit & 0x3fff;
                        if (arg3 || var >= 0) break;
                        fallthrough__;
                    case 2:
                        if ((gSpriteHit[pSpr->extra].hit & 0xc000) == 0xc000) var = gSpriteHit[pSpr->extra].hit & 0x3fff;
                        if (arg3 || var >= 0) break;
                        fallthrough__;
                    case 3:
                        if ((gSpriteHit[pSpr->extra].ceilhit & 0xc000) == 0xc000) var = gSpriteHit[pSpr->extra].ceilhit & 0x3fff;
                        break;
                }
                if (var < 0) { // check if something touching this sprite
                    for (int i = kMaxXSprites - 1, idx = i; i > 0; idx = xsprite[--i].reference) {
                        if (idx < 0 || (sprite[idx].flags & kHitagRespawn)) continue;
                        switch (arg3) {
                            case 0:
                            case 1:
                                if ((gSpriteHit[i].ceilhit & 0xc000) == 0xc000 && (gSpriteHit[i].ceilhit & 0x3fff) == objIndex) var = idx;
                                if (arg3 || var >= 0) break;
                                fallthrough__;
                            case 2:
                                if ((gSpriteHit[i].hit & 0xc000) == 0xc000 && (gSpriteHit[i].hit & 0x3fff) == objIndex) var = idx;
                                if (arg3 || var >= 0) break;
                                fallthrough__;
                            case 3:
                                if ((gSpriteHit[i].florhit & 0xc000) == 0xc000 && (gSpriteHit[i].florhit & 0x3fff) == objIndex) var = idx;
                                break;
                        }
                    }
                }
                if (var < 0) return false;
                else if (PUSH) condPush(pXCond, OBJ_SPRITE, var);
                return true;
            case 65: // compare burn time (in %)
                var = (IsDudeSprite(pSpr)) ? 2400 : 1200;
                if (!condCmp((kPercFull * pXSpr->burnTime) / var, arg1, arg2, cmpOp)) return false;
                else if (PUSH && spriRangeIsFine(pXSpr->burnSource)) condPush(pXCond, OBJ_SPRITE, pXSpr->burnSource);
                return true;
            case 66: // any flares stuck in this sprite?
            {
                int nSprite;
                StatIterator it(kStatFlare);
                while ((nSprite = it.NextIndex()) >= 0)
                {
                    spritetype* pFlare = &sprite[nSprite];
                    if (!xspriRangeIsFine(pFlare->extra) || (pFlare->flags & kHitagFree))
                        continue;

                    XSPRITE* pXFlare = &xsprite[pFlare->extra];
                    if (!spriRangeIsFine(pXFlare->target) || pXFlare->target != objIndex) continue;
                    else if (PUSH) condPush(pXCond, OBJ_SPRITE, nSprite);
                    return true;
                }
                return false;
            }
            case 70:
                return condCmp(getSpriteMassBySize(pSpr), arg1, arg2, cmpOp); // mass of the sprite in a range?
        }
    } else {
        switch (cond) {
            default: return false;
            case 50:
            case 65:
            case 70:
                return condCmpne(arg1, arg2, cmpOp);
        }
    }

    condError(pXCond, "Sprite conditions: Unexpected condition id (%d)!", cond);
    return false;
}

// this updates index of object in all conditions
void condUpdateObjectIndex(int objType, int oldIndex, int newIndex) {

    // update index in tracking conditions first
    for (int i = 0; i < gTrackingCondsCount; i++) {

        TRCONDITION* pCond = &gCondition[i];
        for (unsigned k = 0; k < pCond->length; k++) {
            if (pCond->obj[k].type != objType || pCond->obj[k].index != oldIndex) continue;
            pCond->obj[k].index = newIndex;
            break;
        }

    }

    int oldSerial = condSerialize(objType, oldIndex);
    int newSerial = condSerialize(objType, newIndex);

    // then update serials
    int nSpr;
    StatIterator it(kStatModernCondition);
    while ((nSpr = it.NextIndex()) >= 0)
    {
        XSPRITE* pXCond = &xsprite[sprite[nSpr].extra];
        if (pXCond->targetX == oldSerial) pXCond->targetX = newSerial;
        if (pXCond->targetY == oldSerial) pXCond->targetY = newSerial;

    }

    return;
}

bool valueIsBetween(int val, int min, int max) {
    return (val > min && val < max);
}

char modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState) {
    if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
        return 0;

    pXSprite->busy  = IntToFixed(nState);
    pXSprite->state = nState;
    
    evKill(nSprite, 3);
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);

    if (pXSprite->txID != 0 && ((pXSprite->triggerOn && pXSprite->state) || (pXSprite->triggerOff && !pXSprite->state)))
        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);

    return 1;
}

void modernTypeSendCommand(int nSprite, int destChannel, COMMAND_ID command) {
    switch (command) {
    case kCmdLink:
        evSend(nSprite, 3, destChannel, kCmdModernUse); // just send command to change properties
        return;
    case kCmdUnlock:
        evSend(nSprite, 3, destChannel, command); // send normal command first
        evSend(nSprite, 3, destChannel, kCmdModernUse);  // then send command to change properties
        return;
    default:
        evSend(nSprite, 3, destChannel, kCmdModernUse); // send first command to change properties
        evSend(nSprite, 3, destChannel, command); // then send normal command
        return;
    }
}

// this function used by various new modern types.
void modernTypeTrigger(int destObjType, int destObjIndex, EVENT event) {

    if (event.type != OBJ_SPRITE) return;
    spritetype* pSource = &sprite[event.index];

    if (!xspriRangeIsFine(pSource->extra)) return;
    XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (destObjType) {
        case OBJ_SECTOR:
            if (!xsectRangeIsFine(sector[destObjIndex].extra)) return;
            break;
        case OBJ_WALL:
            if (!xwallRangeIsFine(wall[destObjIndex].extra)) return;
            break;
        case OBJ_SPRITE:
            if (!xspriRangeIsFine(sprite[destObjIndex].extra)) return;
            else if (sprite[destObjIndex].flags & kHitagFree) return;

           /*switch (pSource->type) {
                case kModernEffectSpawner:
                case kModernWindGenerator:
                    switch (sprite[destObjIndex].type) {
                        case kModernEffectSpawner:
                        case kModernWindGenerator:
                            viewSetSystemMessage("SRC %d, DEST %d", Numsprites, sprite[destObjIndex].type);
                            break;
                    }
                    break;
            }*/
            
            // allow redirect events received from some modern types.
            // example: it allows to spawn FX effect if event was received from kModernEffectGen
            // on many TX channels instead of just one.
            switch (sprite[destObjIndex].type) {
                case kModernRandomTX:
                case kModernSequentialTX:
                    spritetype* pSpr = &sprite[destObjIndex]; XSPRITE* pXSpr = &xsprite[pSpr->extra];
                    if (pXSpr->command != kCmdLink) break; // no redirect mode detected
                    else if (!pXSpr->locked) {
                        switch (pSpr->type) {
                            case kModernRandomTX:
                                useRandomTx(pXSpr, (COMMAND_ID)pXSource->command, false); // set random TX id
                                break;
                            case kModernSequentialTX:
                                if (pSpr->flags & kModernTypeFlag1) {
                                    seqTxSendCmdAll(pXSpr, pSource->index, (COMMAND_ID)pXSource->command, true);
                                    return;
                                }
                                useSequentialTx(pXSpr, (COMMAND_ID)pXSource->command, false); // set next TX id
                                break;
                        }
                        if (pXSpr->txID > 0 && pXSpr->txID < kChannelUserMax) {
                            modernTypeSendCommand(pSource->index, pXSpr->txID, (COMMAND_ID)pXSource->command);
                        }
                        return;
                    }
                    break;
            }
            break;
        default:
            return;
    }

    switch (pSource->type) {
        // allows teleport any sprite from any location to the source destination
        case kMarkerWarpDest:
            if (destObjType != OBJ_SPRITE) break;
            useTeleportTarget(pXSource, &sprite[destObjIndex]);
            break;
        // changes slope of sprite or sector
        case kModernSlopeChanger:
            switch (destObjType) {
                case OBJ_SPRITE:
                case OBJ_SECTOR:
                    useSlopeChanger(pXSource, destObjType, destObjIndex);
                    break;
            }
            break;
        case kModernSpriteDamager:
        // damages xsprite via TX ID or xsprites in a sector
            switch (destObjType) {
                case OBJ_SPRITE:
                case OBJ_SECTOR:
                    useSpriteDamager(pXSource, destObjType, destObjIndex);
            break;
            }
            break;
        // can spawn any effect passed in data2 on it's or txID sprite
        case kModernEffectSpawner:
            if (destObjType != OBJ_SPRITE || pXSource->data2 < 0 || pXSource->data2 >= kFXMax) break;
            useEffectGen(pXSource, &sprite[destObjIndex]);
            break;
        // takes data2 as SEQ ID and spawns it on it's or TX ID object
        case kModernSeqSpawner:
            useSeqSpawnerGen(pXSource, destObjType, destObjIndex);
            break;
        // creates wind on TX ID sector
        case kModernWindGenerator:
            if (destObjType != OBJ_SECTOR || pXSource->data2 < 0) break;
            useSectorWindGen(pXSource, &sector[destObjIndex]);
            break;
        // size and pan changer of sprite/wall/sector via TX ID
        case kModernObjSizeChanger:
            useObjResizer(pXSource, destObjType, destObjIndex);
            break;
        // iterate data filed value of destination object
        case kModernObjDataAccumulator:
            useIncDecGen(pXSource, destObjType, destObjIndex);
            break;
        // change data field value of destination object
        case kModernObjDataChanger:
            useDataChanger(pXSource, destObjType, destObjIndex);
            break;
        // change sector lighting dynamically
        case kModernSectorFXChanger:
            if (destObjType != OBJ_SECTOR) break;
            useSectorLigthChanger(pXSource, &xsector[sector[destObjIndex].extra]);
            break;
        // change target of dudes and make it fight
        case kModernDudeTargetChanger:
            if (destObjType != OBJ_SPRITE) break;
            useTargetChanger(pXSource, &sprite[destObjIndex]);
            break;
        // change picture and palette of TX ID object
        case kModernObjPicnumChanger:
            usePictureChanger(pXSource, destObjType, destObjIndex);
            break;
        // change various properties
        case kModernObjPropertiesChanger:
            usePropertiesChanger(pXSource, destObjType, destObjIndex);
            break;
        // updated vanilla sound gen that now allows to play sounds on TX ID sprites
        case kGenModernSound:
            if (destObjType != OBJ_SPRITE) break;
            useSoundGen(pXSource, &sprite[destObjIndex]);
            break;
    }
}

// the following functions required for kModernDudeTargetChanger
//---------------------------------------
spritetype* aiFightGetTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode) {
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type); XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; spritetype* cTarget = NULL;
    int nSprite;
    StatIterator it(kStatDude);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        pTarget = &sprite[nSprite];  pXTarget = &xsprite[pTarget->extra];
        if (!aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) continue;

        int dist = aiFightGetTargetDist(pSprite, pDudeInfo, pTarget);
        if (dist < minDist || dist > maxDist) continue;
        else if (pXSprite->target == pTarget->index) return pTarget;
        else if (!IsDudeSprite(pTarget) || pTarget->index == pSprite->index || IsPlayerSprite(pTarget)) continue;
        else if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || pTarget->owner == pSprite->index) continue;
        else if ((teamMode == 1 && aiFightIsMateOf(pXSprite, pXTarget)) || aiFightMatesHaveSameTarget(pXSprite, pTarget, 1)) continue;
        else if (data == 666 || pXTarget->data1 == data) {

            if (pXSprite->target > 0) {
                cTarget = &sprite[pXSprite->target];
                int fineDist1 = aiFightGetFineTargetDist(pSprite, cTarget);
                int fineDist2 = aiFightGetFineTargetDist(pSprite, pTarget);
                if (fineDist1 < fineDist2)
                    continue;
            }
            return pTarget;
        }
    }

    return NULL;
}

spritetype* aiFightTargetIsPlayer(XSPRITE* pXSprite) {

    if (pXSprite->target >= 0) {
        if (IsPlayerSprite(&sprite[pXSprite->target]))
            return &sprite[pXSprite->target];
    }

    return NULL;
}
spritetype* aiFightGetMateTargets(XSPRITE* pXSprite) {
    int rx = pXSprite->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type == OBJ_SPRITE) {
            pMate = &sprite[rxBucket[i].index];
            if (pMate->extra < 0 || pMate->index == sprite[pXSprite->reference].index || !IsDudeSprite(pMate))
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

bool aiFightMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow) {
    int rx = pXLeader->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {

        if (rxBucket[i].type != OBJ_SPRITE)
            continue;

        pMate = &sprite[rxBucket[i].index];
        if (pMate->extra < 0 || pMate->index == sprite[pXLeader->reference].index || !IsDudeSprite(pMate))
            continue;

        pXMate = &xsprite[pMate->extra];
        if (pXMate->target == pTarget->index && allow-- <= 0)
            return true;
    }

    return false;

}

bool aiFightDudeCanSeeTarget(XSPRITE* pXDude, DUDEINFO* pDudeInfo, spritetype* pTarget) {
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
            if (abs(losAngle) < 2048) // 360 deg periphery here*/
            return true;
        }

    }

    return false;

}

// this function required if monsters in genIdle ai state. It wakes up monsters
// when kModernDudeTargetChanger goes to off state, so they won't ignore the world.
void aiFightActivateDudes(int rx) {
    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type != OBJ_SPRITE) continue;
        spritetype* pDude = &sprite[rxBucket[i].index]; XSPRITE* pXDude = &xsprite[pDude->extra];
        if (!IsDudeSprite(pDude) || pXDude->aiState->stateType != kAiStateGenIdle) continue;
        aiInitSprite(pDude);
    }
}


// this function sets target to -1 for all dudes that hunting for nSprite
void aiFightFreeTargets(int nSprite) {
    int nTarget;
    StatIterator it(kStatDude);
    while ((nTarget = it.NextIndex()) >= 0)
    {
        if (!IsDudeSprite(&sprite[nTarget]) || sprite[nTarget].extra < 0) continue;
        else if (xsprite[sprite[nTarget].extra].target == nSprite)
            aiSetTarget(&xsprite[sprite[nTarget].extra], sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z);
    }

    return;
}

// this function sets target to -1 for all targets that hunting for dudes affected by selected kModernDudeTargetChanger
void aiFightFreeAllTargets(XSPRITE* pXSource) {
    if (pXSource->txID <= 0) return;
    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type == OBJ_SPRITE && sprite[rxBucket[i].index].extra >= 0)
            aiFightFreeTargets(rxBucket[i].index);
    }

    return;
}


bool aiFightDudeIsAffected(XSPRITE* pXDude) {
    if (pXDude->rxID <= 0 || pXDude->locked == 1) return false;
    int nSprite;
    StatIterator it(kStatModernDudeTargetChanger);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        XSPRITE* pXSprite = (sprite[nSprite].extra >= 0) ? &xsprite[sprite[nSprite].extra] : NULL;
        if (pXSprite == NULL || pXSprite->txID <= 0 || pXSprite->state != 1) continue;
        for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
            if (rxBucket[i].type != OBJ_SPRITE) continue;

            spritetype* pSprite = &sprite[rxBucket[i].index];
            if (pSprite->extra < 0 || !IsDudeSprite(pSprite)) continue;
            else if (pSprite->index == sprite[pXDude->reference].index) return true;
        }
    }
    return false;
}

bool aiFightIsMateOf(XSPRITE* pXDude, XSPRITE* pXSprite) {
    return (pXDude->rxID == pXSprite->rxID);
}

// this function tells if there any dude found for kModernDudeTargetChanger
bool aiFightGetDudesForBattle(XSPRITE* pXSprite) {
    
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != OBJ_SPRITE) continue;
        else if (IsDudeSprite(&sprite[rxBucket[i].index]) &&
            xsprite[sprite[rxBucket[i].index].extra].health > 0) return true;
    }

    // check redirected TX buckets
    int rx = -1; XSPRITE* pXRedir = NULL;
    while ((pXRedir = evrListRedirectors(OBJ_SPRITE, sprite[pXSprite->reference].extra, pXRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            if (rxBucket[i].type != OBJ_SPRITE) continue;
            else if (IsDudeSprite(&sprite[rxBucket[i].index]) &&
                xsprite[sprite[rxBucket[i].index].extra].health > 0) return true;
        }
    }
    return false;
}

void aiFightAlarmDudesInSight(spritetype* pSprite, int max) {
    spritetype* pDude = NULL; XSPRITE* pXDude = NULL;
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    int nSprite;
    StatIterator it(kStatDude);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        pDude = &sprite[nSprite];
        if (pDude->index == pSprite->index || !IsDudeSprite(pDude) || pDude->extra < 0)
            continue;
        pXDude = &xsprite[pDude->extra];
        if (aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pDude)) {
            if (pXDude->target != -1 || pXDude->rxID > 0)
                continue;

            aiSetTarget(pXDude, pDude->x, pDude->y, pDude->z);
            aiActivateDude(&bloodActors[pXDude->reference]);
            if (max-- < 1)
                break;
        }
    }
}

bool aiFightUnitCanFly(spritetype* pDude) {
    return (IsDudeSprite(pDude) && gDudeInfoExtra[pDude->type - kDudeBase].flying);
}

bool aiFightIsMeleeUnit(spritetype* pDude) {
    if (pDude->type == kDudeModernCustom) return (pDude->extra >= 0 && dudeIsMelee(&xsprite[pDude->extra]));
    else return (IsDudeSprite(pDude) && gDudeInfoExtra[pDude->type - kDudeBase].melee);
}

int aiFightGetTargetDist(spritetype* pSprite, DUDEINFO* pDudeInfo, spritetype* pTarget) {
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

int aiFightGetFineTargetDist(spritetype* pSprite, spritetype* pTarget) {
    int x = pTarget->x; int y = pTarget->y;
    int dx = x - pSprite->x; int dy = y - pSprite->y;

    int dist = approxDist(dx, dy);
    return dist;
}

int sectorInMotion(int nSector) {
 
    for (int i = 0; i < kMaxBusyCount; i++) {
        if (gBusy->index == nSector) return i;
    }

    return -1;
}

void sectorKillSounds(int nSector) {
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite]) {
        if (sprite[nSprite].type != kSoundSector) continue;
        sfxKill3DSound(&sprite[nSprite]);
    }
}

void sectorPauseMotion(int nSector) {

    if (!xsectRangeIsFine(sector[nSector].extra)) return;
    XSECTOR* pXSector = &xsector[sector[nSector].extra];
    pXSector->unused1 = 1;
    
    evKill(nSector, OBJ_SECTOR);

    sectorKillSounds(nSector);
    if ((pXSector->busy == 0 && !pXSector->state) || (pXSector->busy == 65536 && pXSector->state))
    SectorEndSound(nSector, xsector[sector[nSector].extra].state);
    
    return;
}

void sectorContinueMotion(int nSector, EVENT event) {
    
    if (!xsectRangeIsFine(sector[nSector].extra)) return;
    else if (gBusyCount >= kMaxBusyCount) {
        Printf(PRINT_HIGH, "Failed to continue motion for sector #%d. Max (%d) busy objects count reached!", nSector, kMaxBusyCount);
        return;
    }

    XSECTOR* pXSector = &xsector[sector[nSector].extra];
    pXSector->unused1 = 0;
    
    int busyTimeA = pXSector->busyTimeA;    int waitTimeA = pXSector->waitTimeA;
    int busyTimeB = pXSector->busyTimeB;    int waitTimeB = pXSector->waitTimeB;
    if (sector[nSector].type == kSectorPath) {
        if (!spriRangeIsFine(pXSector->marker0)) return;
        busyTimeA = busyTimeB = xsprite[sprite[pXSector->marker0].extra].busyTime;
        waitTimeA = waitTimeB = xsprite[sprite[pXSector->marker0].extra].waitTime;
    }
    
    if (!pXSector->interruptable && event.cmd != kCmdSectorMotionContinue
        && ((!pXSector->state && pXSector->busy) || (pXSector->state && pXSector->busy != 65536))) {
            
            event.cmd = kCmdSectorMotionContinue;

    } else if (event.cmd == kCmdToggle) {
        
        event.cmd = (pXSector->state) ? kCmdOn : kCmdOff;

    }

    //viewSetSystemMessage("%d / %d", pXSector->busy, pXSector->state);

    int nDelta = 1;
    switch (event.cmd) {
        case kCmdOff:
            if (pXSector->busy == 0) {
                if (pXSector->reTriggerB && waitTimeB) evPost(nSector, OBJ_SECTOR, (waitTimeB * 120) / 10, kCmdOff);
                return;
            }
            pXSector->state = 1;
            nDelta = 65536 / ClipLow((busyTimeB * 120) / 10, 1);
            break;
        case kCmdOn:
            if (pXSector->busy == 65536) {
                if (pXSector->reTriggerA && waitTimeA) evPost(nSector, OBJ_SECTOR, (waitTimeA * 120) / 10, kCmdOn);
                return;
            }
            pXSector->state = 0;
            nDelta = 65536 / ClipLow((busyTimeA * 120) / 10, 1);
            break;
        case kCmdSectorMotionContinue:
            nDelta = 65536 / ClipLow((((pXSector->state) ? busyTimeB : busyTimeA) * 120) / 10, 1);
            break;
    }

    //bool crush = pXSector->Crush;
    int busyFunc = BUSYID_0;
    switch (sector[nSector].type) {
        case kSectorZMotion:
            busyFunc = BUSYID_2;
            break;
        case kSectorZMotionSprite:
            busyFunc = BUSYID_1;
            break;
        case kSectorSlideMarked:
        case kSectorSlide:
            busyFunc = BUSYID_3;
            break;
        case kSectorRotateMarked:
        case kSectorRotate:
            busyFunc = BUSYID_4;
            break;
        case kSectorRotateStep:
            busyFunc = BUSYID_5;
            break;
        case kSectorPath:
            busyFunc = BUSYID_7;
            break;
        default:
            I_Error("Unsupported sector type %d", sector[nSector].type);
            break;
    }

    SectorStartSound(nSector, pXSector->state);
    nDelta = (pXSector->state) ? -nDelta : nDelta;
    gBusy[gBusyCount].index = nSector;
    gBusy[gBusyCount].delta = nDelta;
    gBusy[gBusyCount].busy = pXSector->busy;
    gBusy[gBusyCount].type = (BUSYID)busyFunc;
    gBusyCount++;
    return;

}

bool modernTypeOperateSector(int nSector, sectortype* pSector, XSECTOR* pXSector, EVENT event) {

    if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock) {
        
        switch (event.cmd) {
            case kCmdLock:
                pXSector->locked = 1;
                break;
            case kCmdUnlock:
                pXSector->locked = 0;
                break;
            case kCmdToggleLock:
                pXSector->locked = pXSector->locked ^ 1;
                break;
        }

        switch (pSector->type) {
            case kSectorCounter:
                if (pXSector->locked != 1) break;
                SetSectorState(nSector, pXSector, 0);
                evPost(nSector, 6, 0, kCallbackCounterCheck);
                break;
        }

        return true;
    
    // continue motion of the paused sector
    } else if (pXSector->unused1) {
        
        switch (event.cmd) {
            case kCmdOff:
            case kCmdOn:
            case kCmdToggle:
            case kCmdSectorMotionContinue:
                sectorContinueMotion(nSector, event);
                return true;
        }
    
    // pause motion of the sector
    } else if (event.cmd == kCmdSectorMotionPause) {
        
        sectorPauseMotion(nSector);
        return true;

    }

    return false;

}

bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event) {

    if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLock:
                pXSprite->locked = 1;
                break;
            case kCmdUnlock:
                pXSprite->locked = 0;
                break;
            case kCmdToggleLock:
                pXSprite->locked = pXSprite->locked ^ 1;
                break;
        }

        switch (pSprite->type) {
            case kModernCondition:
            case kModernConditionFalse:
                pXSprite->restState = 0;
                if (pXSprite->busyTime <= 0) break;
                else if (!pXSprite->locked) pXSprite->busy = 0;
                break;
        }
       
        return true;
    } else if (event.cmd == kCmdDudeFlagsSet) {
        
        if (event.type != OBJ_SPRITE) {
           
            viewSetSystemMessage("Only sprites could use command #%d", event.cmd);
            return true;

        } else if (xspriRangeIsFine(sprite[event.index].extra)) {
           
            XSPRITE* pXSource = &xsprite[sprite[event.index].extra];
            pXSprite->dudeFlag4 = pXSource->dudeFlag4;
            pXSprite->dudeAmbush = pXSource->dudeAmbush;
            pXSprite->dudeGuard = pXSource->dudeGuard;
            pXSprite->dudeDeaf = pXSource->dudeDeaf;
            pXSprite->unused1 = pXSource->unused1;

            if (pXSource->unused1 & kDudeFlagStealth) pXSprite->unused1 |= kDudeFlagStealth;
            else pXSprite->unused1 &= ~kDudeFlagStealth;

    }

    }

    if (pSprite->statnum == kStatDude && IsDudeSprite(pSprite)) {
        
        switch (event.cmd) {
            case kCmdOff:
                if (pXSprite->state) SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdOn:
                if (!pXSprite->state) SetSpriteState(nSprite, pXSprite, 1);
                if (IsPlayerSprite(pSprite) || pXSprite->health <= 0) break;
                else if (pXSprite->aiState->stateType >= kAiStatePatrolBase && pXSprite->aiState->stateType < kAiStatePatrolMax)
                    break;

                switch (pXSprite->aiState->stateType) {
                    case kAiStateIdle:
                    case kAiStateGenIdle:
                        aiActivateDude(&bloodActors[pXSprite->reference]);
                        break;
                }
                break;
            case kCmdDudeFlagsSet:
                if (xspriRangeIsFine(sprite[event.index].extra)) {
                    if (!pXSprite->dudeFlag4) {
                        
                if (pXSprite->aiState->stateType < kAiStatePatrolBase || pXSprite->aiState->stateType >= kAiStatePatrolMax) break;
                else aiPatrolStop(pSprite, -1);

                    } else {
                        
                if (pXSprite->aiState->stateType >= kAiStatePatrolBase && pXSprite->aiState->stateType < kAiStatePatrolMax) break;
                else if (spriteIsUnderwater(pSprite)) aiPatrolState(pSprite, kAiStatePatrolWaitW);
                else aiPatrolState(pSprite, kAiStatePatrolWaitL);

                        
                pXSprite->data3 = 0;

                }

                    //viewSetSystemMessage("%d / %d / %d", pSprite->type, pXSprite->dudeFlag4, pXSprite->dudeGuard);
                }
                break;
            default:
                if (!pXSprite->state) evPost(nSprite, 3, 0, kCmdOn);
                else evPost(nSprite, 3, 0, kCmdOff);
                break;
        }

        return true;
    }
    
    switch (pSprite->type) {
        default:
            return false; // no modern type found to work with, go normal OperateSprite();
        case kModernCondition:
        case kModernConditionFalse:
            if (!pXSprite->isTriggered) useCondition(pSprite, pXSprite, event);
            return true;
        // add spawn random dude feature - works only if at least 2 data fields are not empty.
        case kMarkerDudeSpawn:
            if (gGameOptions.nMonsterSettings && pXSprite->data1 >= kDudeBase && pXSprite->data1 < kDudeVanillaMax) {

                spritetype* pSpawn = NULL;
                if ((pSpawn = randomSpawnDude(pSprite)) == NULL
                    && (pSpawn = actSpawnDude(pSprite, pXSprite->data1, -1, 0)) == NULL) {
                        return true;
                }

                XSPRITE* pXSpawn = &xsprite[pSpawn->extra];
                gKillMgr.AddNewKill(1);
                if (IsBurningDude(pSpawn)) {
                    pXSpawn->health = getDudeInfo(pXSprite->data1)->startHealth << 4;
                    pXSpawn->burnTime = 10;
                    pXSpawn->target = -1;
                    if (!pXSpawn->dudeFlag4)
                        aiActivateDude(&bloodActors[pXSpawn->reference]);
                } else if ((pSprite->flags & kModernTypeFlag3) && !pXSpawn->dudeFlag4) {
                    aiActivateDude(&bloodActors[pXSpawn->reference]);
                }
                
            }
            return true;
        case kModernRandomTX: // random Event Switch takes random data field and uses it as TX ID
        case kModernSequentialTX: // sequential Switch takes values from data fields starting from data1 and uses it as TX ID
            if (pXSprite->command == kCmdLink) return true; // work as event redirector
            switch (pSprite->type) {
                case kModernRandomTX:
                    useRandomTx(pXSprite, (COMMAND_ID)pXSprite->command, true);
                    break;
                case kModernSequentialTX:
                    if (!(pSprite->flags & kModernTypeFlag1)) useSequentialTx(pXSprite, (COMMAND_ID)pXSprite->command, true);
                    else seqTxSendCmdAll(pXSprite, pSprite->index, (COMMAND_ID)pXSprite->command, false);
                    break;
            }
            return true;
        case kModernSpriteDamager:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pXSprite->data1 == 0 && sectRangeIsFine(pSprite->sectnum)) useSpriteDamager(pXSprite, OBJ_SECTOR, pSprite->sectnum);
                    else if (pXSprite->data1 >= 666 && pXSprite->data1 < 669) useSpriteDamager(pXSprite, -1, -1);
                    else {

                PLAYER* pPlayer = getPlayerById(pXSprite->data1);
                        if (pPlayer != NULL)
                            useSpriteDamager(pXSprite, OBJ_SPRITE, pPlayer->pSprite->index);
                    }

                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                            break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                            break;
                    }
                return true;
        case kMarkerWarpDest:
            if (pXSprite->txID <= 0) {
               
                PLAYER* pPlayer = getPlayerById(pXSprite->data1);
                if (pPlayer != NULL && SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    useTeleportTarget(pXSprite, pPlayer->pSprite);
                return true;
            }
            fallthrough__;
        case kModernObjPropertiesChanger:
            if (pXSprite->txID <= 0) {
                if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    usePropertiesChanger(pXSprite, -1, -1);
                return true;
            }
            fallthrough__;
        case kModernSlopeChanger:
        case kModernObjSizeChanger:
        case kModernObjPicnumChanger:
        case kModernSectorFXChanger:
        case kModernObjDataChanger:
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return true;
        case kModernCustomDudeSpawn:
            if (gGameOptions.nMonsterSettings && genDudeSpawn(pSprite, -1) != NULL) gKillMgr.AddNewKill(1);
            return true;
        case kModernSeqSpawner:
        case kModernEffectSpawner:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    if (pSprite->type == kModernSeqSpawner) seqSpawnerOffSameTx(pXSprite);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pSprite->type == kModernSeqSpawner) useSeqSpawnerGen(pXSprite, 3, pSprite->index);
                    else useEffectGen(pXSprite, NULL);
            
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, ClipLow((int(pXSprite->busyTime) + Random2(pXSprite->data1)) * 120 / 10, 0), kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernWindGenerator:
            switch (event.cmd) {
                case kCmdOff:
                    windGenStopWindOnSectors(pXSprite);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useSectorWindGen(pXSprite, NULL);

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernDudeTargetChanger:

            // this one is required if data4 of generator was dynamically changed
            // it turns monsters in normal idle state instead of genIdle, so they not ignore the world.
            if (pXSprite->dropMsg == 3 && 3 != pXSprite->data4)
                aiFightActivateDudes(pXSprite->txID);

            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->data4 == 3) aiFightActivateDudes(pXSprite->txID);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID <= 0 || !aiFightGetDudesForBattle(pXSprite)) {
                        aiFightFreeAllTargets(pXSprite);
                        evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    } else {
                        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    }

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            pXSprite->dropMsg = uint8_t(pXSprite->data4);
            return true;
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
                    if (pSprite->flags == kModernTypeFlag0 && incDecGoalValueIsReached(pXSprite)) {
                        evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    }
                    
                    modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
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
                    useRandomItemGen(pSprite, pXSprite);
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernThingTNTProx:
            if (pSprite->statnum != kStatRespawn) {
                switch (event.cmd) {
                case kCmdSpriteProximity:
                    if (pXSprite->state) break;
                    sfxPlay3DSound(pSprite, 452, 0, 0);
                    evPost(nSprite, 3, 30, kCmdOff);
                    pXSprite->state = 1;
                    fallthrough__;
                case kCmdOn:
                    sfxPlay3DSound(pSprite, 451, 0, 0);
                    pXSprite->Proximity = 1;
                    break;
                default:
                    actExplodeSprite(pSprite);
                    break;
                }
            }
            return true;
        case kModernThingEnemyLifeLeech:
            dudeLeechOperate(pSprite, pXSprite, event);
            return true;
        case kModernPlayerControl: { // WIP
            PLAYER* pPlayer = NULL; int cmd = (event.cmd >= kCmdNumberic) ? event.cmd : pXSprite->command;
            if ((pPlayer = getPlayerById(pXSprite->data1)) == NULL
                    || ((cmd < 67 || cmd > 68) && !modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)))
                        return true;

            TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];

            /// !!! COMMANDS OF THE CURRENT SPRITE, NOT OF THE EVENT !!! ///
            if ((cmd -= kCmdNumberic) < 0) return true;
            else if (pPlayer->pXSprite->health <= 0) {
                        
                switch (cmd) {
                    case 36:
                        actHealDude(pPlayer->pXSprite, ((pXSprite->data2 > 0) ? ClipHigh(pXSprite->data2, 200) : getDudeInfo(pPlayer->pSprite->type)->startHealth), 200);
                        pPlayer->curWeapon = 1;
                        break;
                }
                        
                return true;

            }

            switch (cmd) {
                case 0: // 64 (player life form)
                    if (pXSprite->data2 < kModeHuman || pXSprite->data2 > kModeHumanGrown) break;
                    else trPlayerCtrlSetRace(pXSprite, pPlayer);
                    break;
                case 1: // 65 (move speed and jump height)
                    // player movement speed (for all races and postures)
                    if (valueIsBetween(pXSprite->data2, -1, 32767))
                        trPlayerCtrlSetMoveSpeed(pXSprite, pPlayer);

                    // player jump height (for all races and stand posture only)
                    if (valueIsBetween(pXSprite->data3, -1, 32767))
                        trPlayerCtrlSetJumpHeight(pXSprite, pPlayer);
                    break;
                case 2: // 66 (player screen effects)
                    if (pXSprite->data3 < 0) break;
                    else trPlayerCtrlSetScreenEffect(pXSprite, pPlayer);
                    break;
                case 3: // 67 (start playing qav scene)
                    trPlayerCtrlStartScene(pXSprite, pPlayer, (pXSprite->data4 == 1) ? true : false);
                    break;
                case 4: // 68 (stop playing qav scene)
                    if (pXSprite->data2 > 0 && pXSprite->data2 != pPlayer->sceneQav) break;
                    else trPlayerCtrlStopScene(pPlayer);
                    break;
                case 5: // 69 (set player look angle, TO-DO: if tx > 0, take a look on TX ID sprite)
                    //data4 is reserved
                    if (pXSprite->data4 != 0) break;
                    else if (valueIsBetween(pXSprite->data2, -128, 128))
                        trPlayerCtrlSetLookAngle(pXSprite, pPlayer);
                    break;
                case 6: // 70 (erase player stuff...)
                    if (pXSprite->data2 < 0) break;
                    else trPlayerCtrlEraseStuff(pXSprite, pPlayer);
                    break;
                case 7: // 71 (give something to player...)
                    if (pXSprite->data2 <= 0) break;
                    else trPlayerCtrlGiveStuff(pXSprite, pPlayer, pCtrl);
                    break;
                case 8: // 72 (use inventory item)
                    if (pXSprite->data2 < 1 || pXSprite->data2 > 5) break;
                    else trPlayerCtrlUsePackItem(pXSprite, pPlayer, event.cmd);
                    break;
                case 9: // 73 (set player's sprite angle, TO-DO: if tx > 0, take a look on TX ID sprite)
                    //data4 is reserved
                    if (pXSprite->data4 != 0) break;
                    else if (pSprite->flags & kModernTypeFlag1) pPlayer->angle.ang = buildang(pSprite->ang);
                    else if (valueIsBetween(pXSprite->data2, -kAng360, kAng360)) pPlayer->angle.ang = buildang(pXSprite->data2);
                    break;
                case 10: // 74 (print the book)
                    // data2: RFF TXT id
                    // data3: background tile
                    // data4: font base tile
                    // pal: font / background palette
                    // hitag:
                    // d1: 0: print whole text at a time, 1: print line by line, 2: word by word, 3: letter by letter
                    // d2: 1: force pause the game (sp only)
                    // d3: 1: inherit palette for font, 2: inherit palette for background, 3: both
                    // busyTime: speed of word/letter/line printing
                    // waitTime: if TX ID > 0 and TX ID object is book reader, trigger it?
                    break;

            }
        }
        return true;
        case kGenModernSound:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                if (pXSprite->txID)  modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                else useSoundGen(pXSprite, pSprite);
                
                if (pXSprite->busyTime > 0)
                    evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                            break;
            default:
                if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                else evPost(nSprite, 3, 0, kCmdOff);
                            break;
                    }
            return true;
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
                    useUniMissileGen(3, pSprite->extra);
                    if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
    }
}

bool modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event) {
    
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
            return true;
        default:
            return false; // no modern type found to work with, go normal OperateWall();
    }
    
}

bool txIsRanged(XSPRITE* pXSource) {
    if (pXSource->data1 > 0 && pXSource->data2 <= 0 && pXSource->data3 <= 0 && pXSource->data4 > 0) {
        if (pXSource->data1 > pXSource->data4) {
            // data1 must be less than data4
            int tmp = pXSource->data1; pXSource->data1 = pXSource->data4;
            pXSource->data4 = tmp;
        }
        return true;
    }
    return false;
}

void seqTxSendCmdAll(XSPRITE* pXSource, int nIndex, COMMAND_ID cmd, bool modernSend) {
    
    bool ranged = txIsRanged(pXSource);
    if (ranged) {
        for (pXSource->txID = pXSource->data1; pXSource->txID <= pXSource->data4; pXSource->txID++) {
            if (pXSource->txID <= 0 || pXSource->txID >= kChannelUserMax) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    } else {
        for (int i = 0; i <= 3; i++) {
            pXSource->txID = GetDataVal(&sprite[pXSource->reference], i);
            if (pXSource->txID <= 0 || pXSource->txID >= kChannelUserMax) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    }
    
    pXSource->txID = pXSource->sysData1 = 0;
    return;
}

void useRandomTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState) {
    
  
    spritetype* pSource = &sprite[pXSource->reference];
    int tx = 0; int maxRetries = kMaxRandomizeRetries;
    
    if (txIsRanged(pXSource)) {
        while (maxRetries-- >= 0) {
            if ((tx = nnExtRandom(pXSource->data1, pXSource->data4)) != pXSource->txID)
                break;
        }
    } else {
        while (maxRetries-- >= 0) {
            if ((tx = randomGetDataValue(pXSource, kRandomizeTX)) > 0 && tx != pXSource->txID)
                break;
        }
    }

    pXSource->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
    if (setState)
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
        //evSend(pSource->index, OBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);
}

void useSequentialTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    bool range = txIsRanged(pXSource); int cnt = 3; int tx = 0;

    if (range) {
        
        // make sure sysData is correct as we store current index of TX ID here.
        if (pXSource->sysData1 < pXSource->data1) pXSource->sysData1 = pXSource->data1;
        else if (pXSource->sysData1 > pXSource->data4) pXSource->sysData1 = pXSource->data4;

    } else {
        
        // make sure sysData is correct as we store current index of data field here.
        if (pXSource->sysData1 > 3) pXSource->sysData1 = 0;
        else if (pXSource->sysData1 < 0) pXSource->sysData1 = 3;

    }

    switch (cmd) {
        case kCmdOff:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->sysData1-- < 0) pXSource->sysData1 = 3;
                    if ((tx = GetDataVal(pSource, pXSource->sysData1)) <= 0) continue;
                    else break;
                }
            } else {
                if (--pXSource->sysData1 < pXSource->data1) pXSource->sysData1 = pXSource->data4;
                tx = pXSource->sysData1;
            }
            break;
        default:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->sysData1 > 3) pXSource->sysData1 = 0;
                    if ((tx = GetDataVal(pSource, pXSource->sysData1++)) <= 0) continue;
                    else break;
                }
            } else {
                tx = pXSource->sysData1;
                if (pXSource->sysData1 >= pXSource->data4) {
                    pXSource->sysData1 = pXSource->data1;
                    break;
                }
                pXSource->sysData1++;
            }
            break;
    }

    pXSource->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
    if (setState)
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
        //evSend(pSource->index, OBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);
}

int useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT event) {

    int objType = event.type; int objIndex = event.index;
    bool srcIsCondition = false;
    if (objType == OBJ_SPRITE && objIndex != pSource->index)
        srcIsCondition = (sprite[objIndex].type == kModernCondition || sprite[objIndex].type == kModernConditionFalse);

    // if it's a tracking condition, it must ignore all the commands sent from objects
    if (pXSource->busyTime > 0 && event.funcID != kCallbackMax) return -1;
    else if (!srcIsCondition) { // save object serials in the stack and make copy of initial object

        pXSource->targetX = pXSource->targetY = condSerialize(objType, objIndex);

    } else { // or grab serials of objects from previous conditions

        pXSource->targetX = xsprite[sprite[objIndex].extra].targetX;
        pXSource->targetY = xsprite[sprite[objIndex].extra].targetY;

    }
    
    int cond = pXSource->data1; bool ok = false; bool RVRS = (pSource->type == kModernConditionFalse);
    bool RSET = (pXSource->command == kCmdNumberic + 36); bool PUSH = (pXSource->command == kCmdNumberic);
    int comOp = pSource->cstat; // comparison operator
    
    if (pXSource->restState == 0) {

        if (cond == 0) ok = true; // dummy
        else if (cond >= kCondMixedBase && cond < kCondMixedMax) ok = condCheckMixed(pXSource, event, comOp, PUSH);
        else if (cond >= kCondWallBase && cond < kCondWallMax) ok = condCheckWall(pXSource, comOp, PUSH);
        else if (cond >= kCondSectorBase && cond < kCondSectorMax) ok = condCheckSector(pXSource, comOp, PUSH);
        else if (cond >= kCondPlayerBase && cond < kCondPlayerMax) ok = condCheckPlayer(pXSource, comOp, PUSH);
        else if (cond >= kCondDudeBase && cond < kCondDudeMax) ok = condCheckDude(pXSource, comOp, PUSH);
        else if (cond >= kCondSpriteBase && cond < kCondSpriteMax) ok = condCheckSprite(pXSource, comOp, PUSH);
        else condError(pXSource,"Unexpected condition id %d!", cond);

        pXSource->state = (ok ^ RVRS);
        
        if (pXSource->waitTime > 0 && pXSource->state > 0) {

            pXSource->restState = 1;
            evKill(pSource->index, OBJ_SPRITE);
            evPost(pSource->index, OBJ_SPRITE, (pXSource->waitTime * 120) / 10, kCmdRepeat);
            return -1;

        }

    } else if (event.cmd == kCmdRepeat) {

        pXSource->restState = 0;

    } else {
    
        return -1;

    }

    // IF
    if (pXSource->state) {

        pXSource->isTriggered = (pXSource->triggerOnce) ? true : false;

        if (RSET) condRestore(pXSource); // reset focus to the initial object

        // send command to rx bucket
        if (pXSource->txID)
            evSend(pSource->index, OBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);

        if (pSource->flags) {
        
            // send it for object currently in the focus
            if (pSource->flags & kModernTypeFlag1) {
                condUnserialize(pXSource->targetX, &objType, &objIndex);
                nnExtTriggerObject(objType, objIndex, pXSource->command);
            }

            // send it for initial object
            if ((pSource->flags & kModernTypeFlag2) && (pXSource->targetX != pXSource->targetY || !(pSource->hitag & kModernTypeFlag1))) {
                condUnserialize(pXSource->targetY, &objType, &objIndex);
                nnExtTriggerObject(objType, objIndex, pXSource->command);
            }

        }

    // ELSE
    } else if (pXSource->sysData2 >= 0) {
        
        pSource = &sprite[pXSource->sysData2]; pXSource = &xsprite[pSource->extra];
        useCondition(pSource, pXSource, event);
        
        if (pXSource->isTriggered) pXSource->sysData2 = -1;

    }

    return pXSource->state;
}

void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource) {
    // let's first search for previously dropped items and remove it
    if (pXSource->dropMsg > 0) {
        int nItem;
        StatIterator it(kStatItem);
        while ((nItem = it.NextIndex()) >= 0)
        {
            spritetype* pItem = &sprite[nItem];
            if ((unsigned int)pItem->type == pXSource->dropMsg && pItem->x == pSource->x && pItem->y == pSource->y && pItem->z == pSource->z) {
                gFX.fxSpawn((FX_ID)29, pSource->sectnum, pSource->x, pSource->y, pSource->z, 0);
                pItem->type = kSpriteDecoration;
                actPostSprite(nItem, kStatFree);
                break;
            }
        }
    }

    // then drop item
    spritetype* pDrop = randomDropPickupObject(pSource, pXSource->dropMsg);

    // check if generator affected by physics
    if (pDrop != NULL && debrisGetIndex(pSource->index) != -1 && (pDrop->extra >= 0 || dbInsertXSprite(pDrop->index) > 0)) {
        int nIndex = debrisGetFreeIndex();
        if (nIndex >= 0) {
            xsprite[pDrop->extra].physAttr |= kPhysMove | kPhysGravity | kPhysFalling; // must fall always
            pSource->cstat &= ~CSTAT_SPRITE_BLOCK;

            gPhysSpritesList[nIndex] = pDrop->index;
            if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
            getSpriteMassBySize(pDrop); // create mass cache
        }
    }
}

void useUniMissileGen(int, int nXSprite) {

    XSPRITE* pXSprite = &xsprite[nXSprite]; int dx = 0, dy = 0, dz = 0;
    spritetype* pSprite = &sprite[pXSprite->reference];

    if (pXSprite->data1 < kMissileBase || pXSprite->data1 >= kMissileMax)
        return;

    if (pSprite->cstat & 32) {
        if (pSprite->cstat & 8) dz = 0x4000;
        else dz = -0x4000;
    } else {
        dx = CosScale16(pSprite->ang);
        dy = SinScale16(pSprite->ang);
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
            xvel[pMissile->index] = MulScale(velocity, dx, 14);
            yvel[pMissile->index] = MulScale(velocity, dy, 14);
            zvel[pMissile->index] = MulScale(velocity, dz, 14);

        }

        // add bursting for missiles
        if (pMissile->type != kMissileFlareAlt && pXSprite->data4 > 0)
            evPost(pMissile->index, 3, ClipHigh(pXSprite->data4, 500), kCallbackMissileBurst);

    }

}

void useSoundGen(XSPRITE* pXSource, spritetype* pSprite) {
    //spritetype* pSource = &sprite[pXSource->reference];
    int pitch = pXSource->data4 << 1; if (pitch < 2000) pitch = 0;
    sfxPlay3DSoundCP(pSprite, pXSource->data2, -1, 0, pitch, pXSource->data3);
}

void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex) {
    char buffer[5]; int data = -65535; short tmp = 0; int dataIndex = 0;
    sprintf(buffer, "%d", abs(pXSource->data1)); int len = int(strlen(buffer));
    
    for (int i = 0; i < len; i++) {
        dataIndex = (buffer[i] - 52) + 4;
        if ((data = getDataFieldOfObject(objType, objIndex, dataIndex)) == -65535) {
            Printf(PRINT_HIGH, "\nWrong index of data (%c) for IncDec Gen #%d! Only 1, 2, 3 and 4 indexes allowed!\n", buffer[i], objIndex);
            continue;
        }
        spritetype* pSource = &sprite[pXSource->reference];
        
        if (pXSource->data2 < pXSource->data3) {

            data = ClipRange(data, pXSource->data2, pXSource->data3);
            if ((data += pXSource->data4) >= pXSource->data3) {
                switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data > pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2:
                    if (data > pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    tmp = pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                    break;
                case kModernTypeFlag3:
                    if (data > pXSource->data3) data = pXSource->data2;
                    break;
                }
            }

        } else if (pXSource->data2 > pXSource->data3) {

            data = ClipRange(data, pXSource->data3, pXSource->data2);
            if ((data -= pXSource->data4) <= pXSource->data3) {
                switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data < pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2:
                    if (data < pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    tmp = pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                    break;
                case kModernTypeFlag3:
                    if (data < pXSource->data3) data = pXSource->data2;
                    break;
                }
            }
        }

        pXSource->sysData1 = data;
        setDataValueOfObject(objType, objIndex, dataIndex, data);
    }

}


void sprite2sectorSlope(spritetype* pSprite, sectortype* pSector, char rel, bool forcez) {
    
    int slope = 0, z = 0;
    switch (rel) {
        default:
            z = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            if ((pSprite->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) && pSprite->extra > 0 && xsprite[pSprite->extra].Touch) z--;
            slope = pSector->floorheinum;
            break;
        case 1:
            z = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            if ((pSprite->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) && pSprite->extra > 0 && xsprite[pSprite->extra].Touch) z++;
            slope = pSector->ceilingheinum;
            break;
    }

    spriteSetSlope(pSprite->index, slope);
    if (forcez) pSprite->z = z;
}

void useSlopeChanger(XSPRITE* pXSource, int objType, int objIndex) {

    int slope, oslope, i;
    spritetype* pSource = &sprite[pXSource->reference];
    bool flag2 = (pSource->flags & kModernTypeFlag2);

    if (pSource->flags & kModernTypeFlag1) slope = ClipRange(pXSource->data2, -32767, 32767);
    else slope = (32767 / kPercFull) * ClipRange(pXSource->data2, -kPercFull, kPercFull);

    if (objType == OBJ_SECTOR) {
    
        sectortype* pSect = &sector[objIndex];

        switch (pXSource->data1) {
            case 2:
            case 0:
            if (slope == 0) pSect->floorstat &= ~0x0002;
            else if (!(pSect->floorstat & 0x0002))
                    pSect->floorstat |= 0x0002;

            // just set floor slope
            if (flag2) {

                pSect->floorheinum = slope;

            } else {

                // force closest floor aligned sprites to inherit slope of the sector's floor
                for (i = headspritesect[objIndex], oslope = pSect->floorheinum; i != -1; i = nextspritesect[i]) {
                    if (!(sprite[i].cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) continue;
                    else if (getflorzofslope(objIndex, sprite[i].x, sprite[i].y) - kSlopeDist <= sprite[i].z) {

                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 0, true);

                        // set new slope of floor
                        pSect->floorheinum = slope;

                        // force sloped sprites to be on floor slope z
                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 0, true);

                        // restore old slope for next sprite
                        pSect->floorheinum = oslope;

                }
                }

                // finally set new slope of floor
                pSect->floorheinum = slope;

            }

                if (pXSource->data1 == 0) break;
                fallthrough__;
            case 1:
            if (slope == 0) pSect->ceilingstat &= ~0x0002;
            else if (!(pSect->ceilingstat & 0x0002))
                    pSect->ceilingstat |= 0x0002;

            // just set ceiling slope
            if (flag2) {

                pSect->ceilingheinum = slope;

            } else {

                // force closest floor aligned sprites to inherit slope of the sector's ceiling
                for (i = headspritesect[objIndex], oslope = pSect->ceilingheinum; i != -1; i = nextspritesect[i]) {
                    if (!(sprite[i].cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) continue;
                    else if (getceilzofslope(objIndex, sprite[i].x, sprite[i].y) + kSlopeDist >= sprite[i].z) {

                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 1, true);

                        // set new slope of ceiling
                        pSect->ceilingheinum = slope;

                        // force sloped sprites to be on ceiling slope z
                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 1, true);

                        // restore old slope for next sprite
                        pSect->ceilingheinum = oslope;

                }
                }

                // finally set new slope of ceiling
                pSect->ceilingheinum = slope;

            }
                break;
        }

        // let's give a little impulse to the physics sprites...
        for (i = headspritesect[objIndex]; i != -1; i = nextspritesect[i]) {

                if (sprite[i].extra > 0 && xsprite[sprite[i].extra].physAttr > 0) {
                xsprite[sprite[i].extra].physAttr |= kPhysFalling;
                zvel[i]++;
                
            } else if ((sprite[i].statnum == kStatThing || sprite[i].statnum == kStatDude) && (sprite[i].flags & kPhysGravity)) {
                sprite[i].flags |= kPhysFalling;
                zvel[i]++;
                }

            }

    } else if (objType == OBJ_SPRITE) {
        
        spritetype* pSpr = &sprite[objIndex];
        if (!(pSpr->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) pSpr->cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
        if ((pSpr->cstat & CSTAT_SPRITE_ALIGNMENT_SLOPE) != CSTAT_SPRITE_ALIGNMENT_SLOPE)
            pSpr->cstat |= CSTAT_SPRITE_ALIGNMENT_SLOPE;

        switch (pXSource->data4) {
            case 1:
            case 2:
            case 3:
                if (!sectRangeIsFine(pSpr->sectnum)) break;
                switch (pXSource->data4) {
                    case 1: sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 0, flag2); break;
                    case 2: sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 1, flag2); break;
                    case 3:
                        if (getflorzofslope(pSpr->sectnum, pSpr->x, pSpr->y) - kSlopeDist <= pSpr->z) sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 0, flag2);
                        if (getceilzofslope(pSpr->sectnum, pSpr->x, pSpr->y) + kSlopeDist >= pSpr->z) sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 1, flag2);
                        break;
                }
                break;
            default:
        spriteSetSlope(objIndex, slope);
                break;
    }
    }
}

void useDataChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    switch (objType) {
        case OBJ_SECTOR:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
        case OBJ_SPRITE:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data2 != -1 && pXSource->data2 != 32767))
                setDataValueOfObject(objType, objIndex, 2, pXSource->data2);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data3 != -1 && pXSource->data3 != 32767))
                setDataValueOfObject(objType, objIndex, 3, pXSource->data3);

            if ((pSource->flags & kModernTypeFlag1) || pXSource->data4 != 65535)
                setDataValueOfObject(objType, objIndex, 4, pXSource->data4);
            break;
        case OBJ_WALL:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
    }
}

void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    if (valueIsBetween(pXSource->data1, -1, 32767))
        pXSector->wave = ClipHigh(pXSource->data1, 11);

    int oldAmplitude = pXSector->amplitude;
    if (valueIsBetween(pXSource->data2, -128, 128))
        pXSector->amplitude = uint8_t(pXSource->data2);

    if (valueIsBetween(pXSource->data3, -1, 32767))
        pXSector->freq = ClipHigh(pXSource->data3, 255);

    if (valueIsBetween(pXSource->data4, -1, 65535))
        pXSector->phase = ClipHigh(pXSource->data4, 255);

    if (pSource->flags) {
        if (pSource->flags != kModernTypeFlag1) {
            
            pXSector->shadeAlways   = (pSource->flags & 0x0001) ? true : false;
            pXSector->shadeFloor    = (pSource->flags & 0x0002) ? true : false;
            pXSector->shadeCeiling  = (pSource->flags & 0x0004) ? true : false;
            pXSector->shadeWalls    = (pSource->flags & 0x0008) ? true : false;

        } else {

            pXSector->shadeAlways   = true;

        }
    }

    // add to shadeList if amplitude was set to 0 previously
    if (oldAmplitude != pXSector->amplitude && shadeCount < kMaxXSectors) {

        bool found = false;
        for (int i = 0; i < shadeCount; i++) {
            if (shadeList[i] != sector[pXSector->reference].extra) continue;
            found = true;
            break;
        }

        if (!found)
            shadeList[shadeCount++] = sector[pXSector->reference].extra;
    }
}

void useTargetChanger(XSPRITE* pXSource, spritetype* pSprite) {
    
    
    if (!IsDudeSprite(pSprite) || pSprite->statnum != kStatDude) {
        switch (pSprite->type) { // can be dead dude turned in gib
            // make current target and all other dudes not attack this dude anymore
        case kThingBloodBits:
        case kThingBloodChunks:
            aiFightFreeTargets(pSprite->index);
            return;
        default:
            return;
        }
    }
    
    
    //spritetype* pSource = &sprite[pXSource->reference];
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; int receiveHp = 33 + Random(33);
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type); int matesPerEnemy = 1;

    // dude is burning?
    if (pXSprite->burnTime > 0 && spriRangeIsFine(pXSprite->burnSource)) {

        if (IsBurningDude(pSprite)) return;
        else {
            spritetype* pBurnSource = &sprite[pXSprite->burnSource];
            if (pBurnSource->extra >= 0) {
                if (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, &xsprite[pBurnSource->extra])) {
                    pXSprite->burnTime = 0;
                    
                    // heal dude a bit in case of friendly fire
                    int startHp = (pXSprite->sysData2 > 0) ? ClipRange(pXSprite->sysData2 << 4, 1, 65535) : pDudeInfo->startHealth << 4;
                    if (pXSprite->health < (unsigned)startHp) actHealDude(pXSprite, receiveHp, startHp);
                } else if (xsprite[pBurnSource->extra].health <= 0) {
                    pXSprite->burnTime = 0;
                }
            }
        }
    }

    spritetype* pPlayer = aiFightTargetIsPlayer(pXSprite);
    // special handling for player(s) if target changer data4 > 2.
    if (pPlayer != NULL) {
        if (pXSource->data4 == 3) {
            aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
            aiSetGenIdleState(pSprite, pXSprite);
            if (pSprite->type == kDudeModernCustom && leechIsDropped(pSprite))
                removeLeech(leechIsDropped(pSprite));
        } else if (pXSource->data4 == 4) {
            aiSetTarget(pXSprite, pPlayer->x, pPlayer->y, pPlayer->z);
            if (pSprite->type == kDudeModernCustom && leechIsDropped(pSprite))
                removeLeech(leechIsDropped(pSprite));
        }
    }

    int maxAlarmDudes = 8 + Random(8);
    if (pXSprite->target > -1 && sprite[pXSprite->target].extra > -1 && pPlayer == NULL) {
        pTarget = &sprite[pXSprite->target]; pXTarget = &xsprite[pTarget->extra];

        if (aiFightUnitCanFly(pSprite) && aiFightIsMeleeUnit(pTarget) && !aiFightUnitCanFly(pTarget))
            pSprite->flags |= 0x0002;
        else if (aiFightUnitCanFly(pSprite))
            pSprite->flags &= ~0x0002;

        if (!IsDudeSprite(pTarget) || pXTarget->health < 1 || !aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) {
            aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
        }
        // dude attack or attacked by target that does not fit by data id?
        else if (pXSource->data1 != 666 && pXTarget->data1 != pXSource->data1) {
            if (aiFightDudeIsAffected(pXTarget)) {

                // force stop attack target
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                if (pXSprite->burnSource == pTarget->index) {
                    pXSprite->burnTime = 0;
                    pXSprite->burnSource = -1;
                }

                // force stop attack dude
                aiSetTarget(pXTarget, pTarget->x, pTarget->y, pTarget->z);
                if (pXTarget->burnSource == pSprite->index) {
                    pXTarget->burnTime = 0;
                    pXTarget->burnSource = -1;
                }
            }

        }
        else if (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, pXTarget)) {
            spritetype* pMate = pTarget; XSPRITE* pXMate = pXTarget;

            // heal dude
            int startHp = (pXSprite->sysData2 > 0) ? ClipRange(pXSprite->sysData2 << 4, 1, 65535) : pDudeInfo->startHealth << 4;
            if (pXSprite->health < (unsigned)startHp) actHealDude(pXSprite, receiveHp, startHp);

            // heal mate
            startHp = (pXMate->sysData2 > 0) ? ClipRange(pXMate->sysData2 << 4, 1, 65535) : getDudeInfo(pMate->type)->startHealth << 4;
            if (pXMate->health < (unsigned)startHp) actHealDude(pXMate, receiveHp, startHp);

            if (pXMate->target > -1 && sprite[pXMate->target].extra >= 0) {
                pTarget = &sprite[pXMate->target];
                // force mate stop attack dude, if he does
                if (pXMate->target == pSprite->index) {
                    aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                } else if (!aiFightIsMateOf(pXSprite, &xsprite[pTarget->extra])) {
                    // force dude to attack same target that mate have
                    aiSetTarget(pXSprite, pTarget->index);
                    return;

                } else {
                    // force mate to stop attack another mate
                    aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                }
            }

            // force dude stop attack mate, if target was not changed previously
            if (pXSprite->target == pMate->index)
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);


        }
        // check if targets aims player then force this target to fight with dude
        else if (aiFightTargetIsPlayer(pXTarget) != NULL) {
            aiSetTarget(pXTarget, pSprite->index);
        }

        int mDist = 3; if (aiFightIsMeleeUnit(pSprite)) mDist = 2;
        if (pXSprite->target >= 0 && aiFightGetTargetDist(pSprite, pDudeInfo, &sprite[pXSprite->target]) < mDist) {
            if (!isActive(pSprite->index)) aiActivateDude(&bloodActors[pXSprite->reference]);
            return;
        }
        // lets try to look for target that fits better by distance
        else if ((PlayClock & 256) != 0 && (pXSprite->target < 0 || aiFightGetTargetDist(pSprite, pDudeInfo, pTarget) >= mDist)) {
            pTarget = aiFightGetTargetInRange(pSprite, 0, mDist, pXSource->data1, pXSource->data2);
            if (pTarget != NULL) {
                pXTarget = &xsprite[pTarget->extra];

                // Make prev target not aim in dude
                if (pXSprite->target > -1) {
                    spritetype* prvTarget = &sprite[pXSprite->target];
                    aiSetTarget(&xsprite[prvTarget->extra], prvTarget->x, prvTarget->y, prvTarget->z);
                    if (!isActive(pTarget->index))
                        aiActivateDude(&bloodActors[pXTarget->reference]);
                }

                // Change target for dude
                aiSetTarget(pXSprite, pTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(&bloodActors[pXSprite->reference]);

                // ...and change target of target to dude to force it fight
                if (pXSource->data3 > 0 && pXTarget->target != pSprite->index) {
                    aiSetTarget(pXTarget, pSprite->index);
                    if (!isActive(pTarget->index))
                        aiActivateDude(&bloodActors[pXTarget->reference]);
                }

                return;
            }
        }
    }
    
    if ((pXSprite->target < 0 || pPlayer != NULL) && (PlayClock & 32) != 0) {
        // try find first target that dude can see
        int nSprite;
        StatIterator it(kStatDude);
        while ((nSprite = it.NextIndex()) >= 0)
        {
            pTarget = &sprite[nSprite]; pXTarget = &xsprite[pTarget->extra];

            if (pXTarget->target == pSprite->index) {
                aiSetTarget(pXSprite, pTarget->index);
                return;
            }

            // skip non-dudes and players
            if (!IsDudeSprite(pTarget) || (IsPlayerSprite(pTarget) && pXSource->data4 > 0) || pTarget->owner == pSprite->index) continue;
            // avoid self aiming, those who dude can't see, and those who dude own
            else if (!aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget) || pSprite->index == pTarget->index) continue;
            // if Target Changer have data1 = 666, everyone can be target, except AI team mates.
            else if (pXSource->data1 != 666 && pXSource->data1 != pXTarget->data1) continue;
            // don't attack immortal, burning dudes and mates
            if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, pXTarget)))
                continue;

            if (pXSource->data2 == 0 || (pXSource->data2 == 1 && !aiFightMatesHaveSameTarget(pXSprite, pTarget, matesPerEnemy))) {

                // Change target for dude
                aiSetTarget(pXSprite, pTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(&bloodActors[pXSprite->reference]);

                // ...and change target of target to dude to force it fight
                if (pXSource->data3 > 0 && pXTarget->target != pSprite->index) {
                    aiSetTarget(pXTarget, pSprite->index);
                    if (pPlayer == NULL && !isActive(pTarget->index))
                        aiActivateDude(&bloodActors[pXTarget->reference]);

                    if (pXSource->data3 == 2)
                        aiFightAlarmDudesInSight(pTarget, maxAlarmDudes);
                }
                
                return;
            }
            
            break;
        }
    }

    // got no target - let's ask mates if they have targets
    if ((pXSprite->target < 0 || pPlayer != NULL) && pXSource->data2 == 1 && (PlayClock & 64) != 0) {
        spritetype* pMateTarget = NULL;
        if ((pMateTarget = aiFightGetMateTargets(pXSprite)) != NULL && pMateTarget->extra > 0) {
            XSPRITE* pXMateTarget = &xsprite[pMateTarget->extra];
            if (aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pMateTarget)) {
                if (pXMateTarget->target < 0) {
                    aiSetTarget(pXMateTarget, pSprite->index);
                    if (IsDudeSprite(pMateTarget) && !isActive(pMateTarget->index))
                        aiActivateDude(&bloodActors[pXMateTarget->reference]);
                }

                aiSetTarget(pXSprite, pMateTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(&bloodActors[pXSprite->reference]);
                return;

                // try walk in mate direction in case if not see the target
            } else if (pXMateTarget->target >= 0 && aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, &sprite[pXMateTarget->target])) {
                spritetype* pMate = &sprite[pXMateTarget->target];
                pXSprite->target = pMateTarget->index;
                pXSprite->targetX = pMate->x;
                pXSprite->targetY = pMate->y;
                pXSprite->targetZ = pMate->z;
                if (!isActive(pSprite->index))
                    aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
        }
    }
}

void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    //spritetype* pSource = &sprite[pXSource->reference];
    
    switch (objType) {
        case OBJ_SECTOR:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sector[objIndex].floorpicnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].ceilingpicnum = pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sector[objIndex].floorpal = uint8_t(pXSource->data3);

            if (valueIsBetween(pXSource->data4, -1, 65535))
                sector[objIndex].ceilingpal = uint8_t(pXSource->data4);
            break;
        case OBJ_SPRITE:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sprite[objIndex].picnum = pXSource->data1;

            if (pXSource->data2 >= 0) sprite[objIndex].shade = (pXSource->data2 > 127) ? 127 : pXSource->data2;
            else if (pXSource->data2 < -1) sprite[objIndex].shade = (pXSource->data2 < -127) ? -127 : pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sprite[objIndex].pal = uint8_t(pXSource->data3);
            break;
        case OBJ_WALL:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                wall[objIndex].picnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                wall[objIndex].overpicnum = pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                wall[objIndex].pal = uint8_t(pXSource->data3);
            break;
    }
}

//---------------------------------------

// player related
QAV* playerQavSceneLoad(int qavId) {
    QAV* pQav = getQAV(qavId);

    if (!pQav) viewSetSystemMessage("Failed to load QAV animation #%d", qavId);
    return pQav;
}

void playerQavSceneProcess(PLAYER* pPlayer, QAVSCENE* pQavScene) {
    int nIndex = pQavScene->index;
    if (xspriRangeIsFine(sprite[nIndex].extra)) {
        
        XSPRITE* pXSprite = &xsprite[sprite[nIndex].extra];
        if (pXSprite->waitTime > 0 && --pXSprite->sysData1 <= 0) {
            if (pXSprite->txID >= kChannelUser) {
                
                XSPRITE* pXSpr = NULL;
                for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
                    if (rxBucket[i].type == OBJ_SPRITE) {
                        
                        spritetype* pSpr = &sprite[rxBucket[i].index];
                        if (pSpr->index == nIndex || !xspriRangeIsFine(pSpr->extra))
                            continue;

                        pXSpr = &xsprite[pSpr->extra];
                        if (pSpr->type == kModernPlayerControl && pXSpr->command == 67) {
                            if (pXSpr->data2 == pXSprite->data2 || pXSpr->locked) continue;
                            else trPlayerCtrlStartScene(pXSpr, pPlayer, true);
                            return;
                        }

                    }

                    nnExtTriggerObject(rxBucket[i].type, rxBucket[i].index, pXSprite->command);

                }
            } //else {
                
                trPlayerCtrlStopScene(pPlayer);

            //}

        } else {
            
            playerQavScenePlay(pPlayer);
            pPlayer->weaponTimer = ClipLow(pPlayer->weaponTimer -= 4, 0);

        }
    } else {
        
        pQavScene->index = pPlayer->sceneQav = -1;
        pQavScene->qavResrc = NULL;
    }
}

void playerQavSceneDraw(PLAYER* pPlayer, int a2, double a3, double a4, int a5, int smoothratio) {
    if (pPlayer == NULL || pPlayer->sceneQav == -1) return;

    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    spritetype* pSprite = &sprite[pQavScene->index];

    if (pQavScene->qavResrc != NULL) {

        QAV* pQAV = pQavScene->qavResrc;
        int v4 = (pPlayer->weaponTimer == 0) ? ((PlayClock + MulScale(4, smoothratio, 16)) % pQAV->duration) : pQAV->duration - pPlayer->weaponTimer;

        int flags = 2; int nInv = powerupCheck(pPlayer, kPwUpShadowCloak);
        if (nInv >= 120 * 8 || (nInv != 0 && (PlayClock & 32))) {
            a2 = -128; flags |= 1;
        }

        // draw as weapon
        if (!(pSprite->flags & kModernTypeFlag1)) {

            pQAV->x = int(a3); pQAV->y = int(a4);
            pQAV->Draw(a3, a4, v4, flags, a2, a5, true);

            // draw fullscreen (currently 4:3 only)
        } else {
            // What an awful hack. This throws proper ordering out of the window, but there is no way to reproduce this better with strict layering of elements.
			// From the above commit it seems to be incomplete anyway...
            pQAV->Draw(v4, flags, a2, a5, false);
        }

    }

}

void playerQavScenePlay(PLAYER* pPlayer) {
    if (pPlayer == NULL) return;
    
    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    if (pPlayer->sceneQav == -1 && pQavScene->index >= 0)
        pPlayer->sceneQav = xsprite[sprite[pQavScene->index].extra].data2;

    if (pQavScene->qavResrc != NULL) {
        QAV* pQAV = pQavScene->qavResrc;
        pQAV->nSprite = pPlayer->pSprite->index;
        int nTicks = pQAV->duration - pPlayer->weaponTimer;
        pQAV->Play(nTicks - 4, nTicks, pPlayer->qavCallback, pPlayer);
    }
}

void playerQavSceneReset(PLAYER* pPlayer) {
    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    pQavScene->index = pQavScene->dummy = pPlayer->sceneQav = -1;
    pQavScene->qavResrc = NULL;
}

bool playerSizeShrink(PLAYER* pPlayer, int divider) {
    pPlayer->pXSprite->scale = 256 / divider;
    playerSetRace(pPlayer, kModeHumanShrink);
    return true;
}

bool playerSizeGrow(PLAYER* pPlayer, int multiplier) {
    pPlayer->pXSprite->scale = 256 * multiplier;
    playerSetRace(pPlayer, kModeHumanGrown);
    return true;
}

bool playerSizeReset(PLAYER* pPlayer) {
    playerSetRace(pPlayer, kModeHuman);
    pPlayer->pXSprite->scale = 0;
    return true;
}

void playerDeactivateShrooms(PLAYER* pPlayer) {
    powerupDeactivate(pPlayer, kPwUpGrowShroom);
    pPlayer->pwUpTime[kPwUpGrowShroom] = 0;

    powerupDeactivate(pPlayer, kPwUpShrinkShroom);
    pPlayer->pwUpTime[kPwUpShrinkShroom] = 0;
}



PLAYER* getPlayerById(short id) {

    // relative to connected players
    if (id >= 1 && id <= kMaxPlayers) {
        id = id - 1;
        for (int i = connecthead; i >= 0; i = connectpoint2[i]) {
            if (id == gPlayer[i].nPlayer)
                return &gPlayer[i];
        }

    // absolute sprite type
    } else if (id >= kDudePlayer1 && id <= kDudePlayer8) {
        for (int i = connecthead; i >= 0; i = connectpoint2[i]) {
            if (id == gPlayer[i].pSprite->type)
                return &gPlayer[i];
        }
    }

    //viewSetSystemMessage("There is no player id #%d", id);
    return NULL;
}

// misc functions
bool IsBurningDude(spritetype* pSprite) {
    if (pSprite == NULL) return false;
    switch (pSprite->type) {
    case kDudeBurningInnocent:
    case kDudeBurningCultist:
    case kDudeBurningZombieAxe:
    case kDudeBurningZombieButcher:
    case kDudeBurningTinyCaleb:
    case kDudeBurningBeast:
    case kDudeModernCustomBurning:
        return true;
    }

    return false;
}

bool IsKillableDude(spritetype* pSprite) {
    switch (pSprite->type) {
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        return false;
    default:
        if (!IsDudeSprite(pSprite) || xsprite[pSprite->extra].locked == 1) return false;
        return true;
    }
}

bool isGrown(spritetype* pSprite) {
    if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpGrowShroom) > 0) return true;
    else if (pSprite->extra >= 0 && xsprite[pSprite->extra].scale >= 512) return true;
    else return false;
}

bool isShrinked(spritetype* pSprite) {
    if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpShrinkShroom) > 0) return true;
    else if (pSprite->extra >= 0 && xsprite[pSprite->extra].scale > 0 && xsprite[pSprite->extra].scale <= 128) return true;
    else return false;
}

bool isActive(int nSprite) {
    if (sprite[nSprite].extra < 0 || sprite[nSprite].extra >= kMaxXSprites)
        return false;

    XSPRITE* pXDude = &xsprite[sprite[nSprite].extra];
    switch (pXDude->aiState->stateType) {
    case kAiStateIdle:
    case kAiStateGenIdle:
    case kAiStateSearch:
    case kAiStateMove:
    case kAiStateOther:
        return false;
    default:
        return true;
    }
}

int getDataFieldOfObject(int objType, int objIndex, int dataIndex) {
    int data = -65535;
    switch (objType) {
        case OBJ_SPRITE:
            switch (dataIndex) {
                case 1: return xsprite[sprite[objIndex].extra].data1;
                case 2: return xsprite[sprite[objIndex].extra].data2;
                case 3:
                    switch (sprite[objIndex].type) {
                        case kDudeModernCustom: return xsprite[sprite[objIndex].extra].sysData1;
                        default: return xsprite[sprite[objIndex].extra].data3;
                    }
                case 4:return xsprite[sprite[objIndex].extra].data4;
                default: return data;
            }
        case OBJ_SECTOR: return xsector[sector[objIndex].extra].data;
        case OBJ_WALL: return xwall[wall[objIndex].extra].data;
        default: return data;
    }
}

bool setDataValueOfObject(int objType, int objIndex, int dataIndex, int value) {
    switch (objType) {
        case OBJ_SPRITE: {
            XSPRITE* pXSprite = &xsprite[sprite[objIndex].extra];

            // exceptions
            if (IsDudeSprite(&sprite[objIndex]) && pXSprite->health <= 0) return true;
            switch (sprite[objIndex].type) {
                case kThingBloodBits:
                case kThingBloodChunks:
                case kThingZombieHead:
                    return true;
                    break;
            }

            switch (dataIndex) {
                case 1:
                    xsprite[sprite[objIndex].extra].data1 = value;
                    switch (sprite[objIndex].type) {
                        case kSwitchCombo:
                            if (value == xsprite[sprite[objIndex].extra].data2) SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 1);
                            else SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 0);
                            break;
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyWeapon] = true;
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
                            evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
                            break;
                    }
                    return true;
                case 2:
                    xsprite[sprite[objIndex].extra].data2 = value;
                    switch (sprite[objIndex].type) {
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertySpriteSize] = true;
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyMass] = true;
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyStates] = true;
                            gGenDudeExtra[objIndex].updReq[kGenDudePropertyAttack] = true;
                            evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
                            break;
                    }
                    return true;
                case 3:
                    xsprite[sprite[objIndex].extra].data3 = value;
                    switch (sprite[objIndex].type) {
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            xsprite[sprite[objIndex].extra].sysData1 = value;
                            break;
                    }
                    return true;
                case 4:
                    xsprite[sprite[objIndex].extra].data4 = value;
                    return true;
                default:
                    return false;
            }
        }
        case OBJ_SECTOR:
            xsector[sector[objIndex].extra].data = value;
            return true;
        case OBJ_WALL:
            xwall[wall[objIndex].extra].data = value;
            return true;
        default:
            return false;
    }
}

/// patrol functions
// ------------------------------------------------

AISTATE* aiInPatrolState(AISTATE* pAiState) {
    
    for (int i = 0; i < kPatrolStateSize; i++) {
        if (pAiState == &genPatrolStates[i])
            return pAiState;
    }
    return NULL;

}

bool aiPatrolCrouching(AISTATE* pAiState) {
    return (pAiState->stateType == kAiStatePatrolWaitC || pAiState->stateType == kAiStatePatrolMoveC);
}

bool aiPatrolWaiting(AISTATE* pAiState) {
    return (pAiState->stateType == kAiStatePatrolWaitL || pAiState->stateType == kAiStatePatrolWaitW || pAiState->stateType == kAiStatePatrolWaitC);
}

bool aiPatrolMoving(AISTATE* pAiState) {
    return (pAiState->stateType == kAiStatePatrolMoveL || pAiState->stateType == kAiStatePatrolMoveW || pAiState->stateType == kAiStatePatrolMoveC);
}

void aiPatrolState(spritetype* pSprite, int state) {

    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    XSPRITE* pXSprite = &xsprite[pSprite->extra];

    int seq = -1, i, start, end; bool crouch;
    DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[pSprite->type - kDudeBase];
    switch (state) {
        case kAiStatePatrolWaitL:
            seq = pExtra->idlgseqofs;
            start = 0; end = 2;
            crouch = false;
            break;
        case kAiStatePatrolMoveL:
            seq = pExtra->mvegseqofs;
            start = 2; end = 7;
            crouch = false;
            break;
        case kAiStatePatrolWaitW:
            seq = pExtra->idlwseqofs;
            start = 7; end = 13;
            crouch = false;
            break;
        case kAiStatePatrolMoveW:
            seq = pExtra->mvewseqofs;
            start = 13; end = 20;
            crouch = false;
            break;
        case kAiStatePatrolWaitC:
            seq = pExtra->idlcseqofs;
            start = 20; end = 24;
            crouch = true;
            break;
        case kAiStatePatrolMoveC:
            seq = pExtra->mvecseqofs;
            start = 24; end = kPatrolStateSize;
            crouch = true;
            break;
    }

    if (seq < 0)
        return aiPatrolStop(pSprite, -1);

    for (i = start; i < end; i++) {

        AISTATE* curState = &genPatrolStates[i];
        if (curState->stateType != state || seq != curState->seqId) continue;
        aiChooseDirection(pSprite, pXSprite, getangle(pXSprite->targetX - pSprite->x, pXSprite->targetY - pSprite->y));
        if (pSprite->type == kDudeModernCustom) aiGenDudeNewState(pSprite, &genPatrolStates[i]);
        else aiNewState(&bloodActors[pXSprite->reference], &genPatrolStates[i]);

        if (crouch) pXSprite->unused1 |= kDudeFlagCrouch;
        else pXSprite->unused1 &= ~kDudeFlagCrouch;

        // these don't have idle crouch seq for some reason...
        if (state == kAiStatePatrolWaitC && (pSprite->type == kDudeCultistTesla || pSprite->type == kDudeCultistTNT)) {
            seqKill(OBJ_SPRITE, pSprite->extra);
            pSprite->picnum = 3385; // set idle picnum
        }

        return;

    }

    if (i == end) {
        viewSetSystemMessage("No patrol state #%d found for dude #%d (type = %d)", state, pSprite->index, pSprite->type);
        aiPatrolStop(pSprite, -1);
    }
}

// check if some dude already follows the given marker
int aiPatrolMarkerBusy(int nExcept, int nMarker) {
    for (int i = headspritestat[kStatDude]; i != -1; i = nextspritestat[i]) {
        if (!IsDudeSprite(&sprite[i]) || sprite[i].index == nExcept || !xspriRangeIsFine(sprite[i].extra))
            continue;

        XSPRITE* pXDude = &xsprite[sprite[i].extra];
        if (pXDude->health > 0 && pXDude->target >= 0 && sprite[pXDude->target].type == kMarkerPath && pXDude->target == nMarker)
            return sprite[i].index;
    }

    return -1;
}

bool aiPatrolMarkerReached(spritetype* pSprite, XSPRITE* pXSprite) {

    if (pSprite->type >= kDudeBase && pSprite->type < kDudeMax) {

        DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[pSprite->type - kDudeBase];
        if (spriRangeIsFine(pXSprite->target) && sprite[pXSprite->target].type == kMarkerPath) {
            short okDist = ClipLow(sprite[pXSprite->target].clipdist << 1, 4);
            if (spriteIsUnderwater(pSprite) || pExtra->flying)
                return CheckProximity(&sprite[pXSprite->target], pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, okDist);

            // ignore z of marker for ground
            spritetype* pMarker = &sprite[pXSprite->target];
            int oX = abs(pMarker->x - pSprite->x) >> 4;
            int oY = abs(pMarker->y - pSprite->y) >> 4;
            return (approxDist(oX, oY) <= okDist);
        }

    }

    return false;

}

void aiPatrolSetMarker(spritetype* pSprite, XSPRITE* pXSprite) {

    int path = -1; int next = -1; int i = 0;
    if (pXSprite->target <= 0) {

        long closest = 20000000; // select closest marker that dude can see
        for (i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i]) {

            long dist = 0;
            int dx = sprite[i].x - pSprite->x;
            int dy = sprite[i].y - pSprite->y;
            int eyeAboveZ = (getDudeInfo(pSprite->type)->eyeHeight * pSprite->yrepeat) << 2;
            if (cansee(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum, pSprite->x, pSprite->y, pSprite->z - eyeAboveZ, pSprite->sectnum)
                && (dist = approxDist(dx, dy)) <= closest && xspriRangeIsFine(sprite[i].extra) && !xsprite[sprite[i].extra].locked
                && !xsprite[sprite[i].extra].DudeLockout) {

                closest = dist;
                path = i;

            }

        }

    // set next marker
    } else if (sprite[pXSprite->target].type == kMarkerPath && xspriRangeIsFine(sprite[pXSprite->target].extra)) {

        int total = 0, random = 0; next = xsprite[sprite[pXSprite->target].extra].data2;
        for (i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i]) {
            if (!xspriRangeIsFine(sprite[i].extra) || xsprite[sprite[i].extra].data1 != next) continue;
            else if (!xsprite[sprite[i].extra].locked && !xsprite[sprite[i].extra].DudeLockout)
                total++;
        }

        if (total <= 0) {
            //viewSetSystemMessage("Follow: No markers with id #%d found for dude #%d!", next, pSprite->index);
            return;
        }

        random = nnExtRandom(0, total);
        for (i = headspritestat[kStatPathMarker]; i >= 0; i = nextspritestat[i]) {
            if (sprite[i].index == pXSprite->target || !xspriRangeIsFine(sprite[i].extra) || xsprite[sprite[i].extra].data1 != next) continue;
            else if (xsprite[sprite[i].extra].locked || xsprite[sprite[i].extra].DudeLockout) continue;
            else if (total > 1 && (random != total-- || (aiPatrolMarkerBusy(pSprite->index, sprite[i].index) >= 0 && !Chance(0x0500)))) continue;
            path = sprite[i].index;
            break;
        }
    }

    if (!spriRangeIsFine(path))
        return;

    pXSprite->target = path;
    pXSprite->targetX = sprite[path].x;
    pXSprite->targetY = sprite[path].y;
    pXSprite->targetZ = sprite[path].z;
    sprite[path].owner = pSprite->index;

}

void aiPatrolStop(spritetype* pSprite, int target, bool alarm) {
    if (xspriRangeIsFine(pSprite->extra)) {

        XSPRITE* pXSprite = &xsprite[pSprite->extra];
        pXSprite->data3 = 0; // reset spot progress
        pXSprite->unused1 &= kDudeFlagCrouch; // reset the crouch status
        if (pXSprite->health <= 0)
            return;

        if (pXSprite->target >= 0 && sprite[pXSprite->target].type == kMarkerPath) {
            if (target < 0) pSprite->ang = sprite[pXSprite->target].ang & 2047;
            pXSprite->target = -1;
        }

        bool flag4 = pXSprite->dudeFlag4;
        pXSprite->dudeFlag4 = 0;
        if (spriRangeIsFine(target) && IsDudeSprite(&sprite[target]) && xspriRangeIsFine(sprite[target].extra)) {

            aiSetTarget(pXSprite, target);
            aiActivateDude(&bloodActors[pXSprite->reference]);
            if (alarm)
                aiPatrolAlarm(pSprite, Chance(0x0500));

        } else {

            
            aiInitSprite(pSprite);
            aiSetTarget(pXSprite, pXSprite->targetX, pXSprite->targetY, pXSprite->targetZ);
            

        }
        pXSprite->dudeFlag4 = flag4; // this must be kept so enemy can patrol after respawn again
    }
    return;
}

void aiPatrolMoveZ(spritetype* pSprite, XSPRITE* pXSprite) {
    if (!spriRangeIsFine(pXSprite->target))
        return;

    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    spritetype* pTarget = &sprite[pXSprite->target];

    int z = pSprite->z + pDudeInfo->eyeHeight;
    int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
    int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
    pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
    if (abs(nAng) > 341) {
        pSprite->ang = (pSprite->ang + 512) & 2047;
        return;
    }

    int dz = (pTarget->z - z) * 5;
    zvel[pSprite->index] = dz;

}


void aiPatrolMove(DBloodActor* actor) {
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();

    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
        return;

    int dudeIdx;
    switch (pSprite->type) {
        default:                        dudeIdx = pSprite->type - kDudeBase;        break;
        case kDudeCultistShotgunProne:  dudeIdx = kDudeCultistShotgun - kDudeBase;  break;
        case kDudeCultistTommyProne:    dudeIdx = kDudeCultistTommy - kDudeBase;    break;
    }

    DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[dudeIdx];
    if (pExtra->flying || spriteIsUnderwater(pSprite)) {
        pSprite->flags &= ~kPhysGravity;
        aiPatrolMoveZ(pSprite, pXSprite);
    
    } else if (!pExtra->flying) {
        pSprite->flags |= kPhysGravity | kPhysFalling;
    }

    if (pSprite->type == kDudeModernCustom) {
        
        aiGenDudeMoveForward(&bloodActors[pXSprite->reference]);

    } else {

        DUDEINFO* pDudeInfo = &dudeInfo[dudeIdx];
        int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
        int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
        int frontSpeed = pDudeInfo->frontSpeed;

        pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
        if (abs(nAng) <= 341) {
            xvel[pSprite->index] += MulScale(frontSpeed, Cos(pSprite->ang), 30);
            yvel[pSprite->index] += MulScale(frontSpeed, Sin(pSprite->ang), 30);
        }

    }
    
    int vel = (aiPatrolCrouching(pXSprite->aiState)) ? kMaxPatrolCrouchVelocity : kMaxPatrolVelocity;
    xvel[pSprite->index] = ClipRange(xvel[pSprite->index], -vel, vel);
    yvel[pSprite->index] = ClipRange(yvel[pSprite->index], -vel, vel);
    return;
}

void aiPatrolAlarm(spritetype* pSprite, bool chain) {

    static short chainChance = 0;
    if (chainChance <= 0) chainChance = 0x1000;
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    spritetype* pDude = NULL; XSPRITE* pXDude = NULL;
    int target = pXSprite->target;
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pDude = &sprite[nSprite];
        if (pDude->index == pSprite->index || !IsDudeSprite(pDude) || IsPlayerSprite(pDude) || pDude->extra < 0) continue;
        //else if (pDude->type == kDudeGargoyleStatueFlesh || pDude->type == kDudeGargoyleStatueStone)
            //continue;

        pXDude = &xsprite[pDude->extra];
        if (pXDude->health > 0 && approxDist(pDude->x - pSprite->x, pDude->y - pSprite->y) < pDudeInfo->seeDist) {
            if (aiInPatrolState(pXDude->aiState)) aiPatrolStop(pDude, pXDude->target);
            if (pXDude->target >= 0 || pXDude->target == pXSprite->target)
                continue;

            if (spriRangeIsFine(target)) aiSetTarget(pXDude, target);
            else aiSetTarget(pXDude, pXSprite->targetX, pXSprite->targetY, pXSprite->targetZ);

            aiActivateDude(&bloodActors[pXDude->reference]);
            if (chain) {
                aiPatrolAlarm(pDude, Chance(chainChance));
                chainChance -= 0x0100;
            }

            DPrintf(DMSG_SPAMMY, "Dude #%d alarms dude #%d", pSprite->index, pDude->index);
        }
    }

}

bool isTouchingSprite(int nXSprite1, int nXSprite2) {

    if (!xspriRangeIsFine(nXSprite1) || !xspriRangeIsFine(nXSprite2))
        return false;

    int nHSprite = -1;
    if ((gSpriteHit[nXSprite1].hit & 0xc000) == 0xc000) nHSprite = gSpriteHit[nXSprite1].hit & 0x3fff;
    else if ((gSpriteHit[nXSprite1].florhit & 0xc000) == 0xc000) nHSprite = gSpriteHit[nXSprite1].florhit & 0x3fff;
    else if ((gSpriteHit[nXSprite1].ceilhit & 0xc000) == 0xc000) nHSprite = gSpriteHit[nXSprite1].ceilhit & 0x3fff;
    return (spriRangeIsFine(nHSprite) && sprite[nHSprite].extra == nXSprite2);
}

bool aiCanCrouch(spritetype* pSprite) {
    
    if (pSprite->type >= kDudeBase && pSprite->type < kDudeVanillaMax)
        return (gDudeInfoExtra[pSprite->type - kDudeBase].idlcseqofs >= 0 && gDudeInfoExtra[pSprite->type - kDudeBase].mvecseqofs >= 0);
    else if (pSprite->type == kDudeModernCustom || pSprite->type == kDudeModernCustomBurning)
        return gGenDudeExtra[pSprite->index].canDuck;

    return false;

}


bool readyForCrit(spritetype* pHunter, spritetype* pVictim) {

    if (!(pHunter->type >= kDudeBase && pHunter->type < kDudeMax) || !(pVictim->type >= kDudeBase && pVictim->type < kDudeMax))
        return false;

    int x, y, dx, dy, nDist;
    x = pVictim->x;
    y = pVictim->y;

    
    dx = x - pHunter->x;
    dy = y - pHunter->y;
    if ((nDist = approxDist(dx, dy)) >= (6000 / ClipLow(gGameOptions.nDifficulty >> 1, 1)))
        return false;

    DUDEINFO* pDudeInfo = getDudeInfo(pVictim->type);
    int nDeltaAngle = ((getangle(dx, dy) + 1024 - pVictim->ang) & 2047) - 1024;
    return (abs(nDeltaAngle) < (pDudeInfo->periphery >> 1));
}

int aiPatrolSearchTargets(spritetype* pSprite, XSPRITE* pXSprite) {
   
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    int i, x, y, z, dx, dy, nDist, eyeAboveZ, target = -1, seeDist, hearDist, seeChance, hearChance;
    bool stealth = (pXSprite->unused1 & kDudeFlagStealth);
    PLAYER* pPlayer = NULL;

    // search for targets
    for (i = headspritestat[kStatDude]; i >= 0; i = nextspritestat[i]) {

        target = -1;
        seeChance = hearChance = 0x0000;

        spritetype* pSpr = &sprite[i];
        if (!xspriRangeIsFine(pSpr->extra))
            continue;

        XSPRITE* pXSpr = &xsprite[pSpr->extra];
        if (pSprite->index == pSpr->index || pSprite->owner == pSpr->index || pXSpr->health == 0)
            continue;

        x = pSpr->x;
        y = pSpr->y;
        z = pSpr->z;

        dx = x - pSprite->x;
        dy = y - pSprite->y;
        nDist = approxDist(dx, dy);
        
        hearDist = pDudeInfo->hearDist >> 1;
        seeDist = (stealth) ? pDudeInfo->seeDist / 3 : pDudeInfo->seeDist >> 1;

        eyeAboveZ = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;
        if (nDist > seeDist || !cansee(x, y, z, pSpr->sectnum, pSprite->x, pSprite->y, pSprite->z - eyeAboveZ, pSprite->sectnum)) continue;
        else if ((pPlayer = getPlayerById(pSpr->type)) == NULL) { // check if this dude is a target for some others

            if (pXSpr->target != pSprite->index) continue;
            else return pSpr->index;

        }

        bool invisible = (powerupCheck(pPlayer, kPwUpShadowCloak) > 0);
        int periphery = ClipLow(pDudeInfo->periphery, kAng90);
        int nDeltaAngle = 1024;
        
        if (!invisible) {
            
            if (stealth) {
            switch (pPlayer->lifeMode) {
                case kModeHuman:
                case kModeHumanShrink:
                    if (pPlayer->lifeMode == kModeHumanShrink) {
                        seeDist -= mulscale8(164, seeDist);
                        hearDist -= mulscale8(164, hearDist);
                    }
                    if (pPlayer->posture == kPostureCrouch) {
                        seeDist -= mulscale8(64, seeDist);
                        hearDist -= mulscale8(128, hearDist);
                    }
                    break;
                case kModeHumanGrown:
                    if (pPlayer->posture != kPostureCrouch) {
                        seeDist += mulscale8(72, seeDist);
                        hearDist += mulscale8(64, hearDist);
                    } else {
                        seeDist += mulscale8(48, seeDist);
                    }
                    break;
            }
            }
            
            seeDist = ClipLow(seeDist, 0);
            hearDist = ClipLow(hearDist, 0);
            if (!pXSprite->dudeDeaf)
                hearChance = mulscale8(1, ClipLow(((hearDist - nDist) + (abs(xvel[pSpr->index]) + abs(yvel[pSpr->index]) + abs(zvel[pSpr->index]))) >> 6, 0));

            if (!pXSprite->dudeGuard) {
                seeChance = 100 - mulscale8(ClipRange(5 - gGameOptions.nDifficulty, 1, 4), nDist >> 1);
                nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
            }

        }

        if (!isTouchingSprite(pSprite->extra, pSpr->extra) && !isTouchingSprite(pSpr->extra, pSprite->extra)) {

            if (nDist < hearDist && hearChance > 0) {
                DPrintf(DMSG_SPAMMY, "Patrol dude #%d hearing the Player #%d.", pSprite->index, pPlayer->nPlayer + 1);
                pXSprite->data3 += hearChance;
                if (!stealth) {
                    target = pSpr->index;
                    break;
            }
            }

            if (nDist < seeDist && abs(nDeltaAngle) < periphery && seeChance > 0) {
                DPrintf(DMSG_SPAMMY, "Patrol dude #%d seeing the Player #%d.", pSprite->index, pPlayer->nPlayer + 1);
                pXSprite->data3 += seeChance;
                if (!stealth) {
                    target = pSpr->index;
                    break;
            }
            }

            if ((pXSprite->data3 = ClipRange(pXSprite->data3, 0, kMaxPatrolSpotValue)) == kMaxPatrolSpotValue) {
                target = pSpr->index;
                break;
            }


        } else {
            
            DPrintf(DMSG_SPAMMY, "Patrol dude #%d spot the Player #%d via touch.", pSprite->index, pPlayer->nPlayer + 1);
            if (invisible) pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
            target = pSpr->index;
            break;
        
        }
        
        //int perc = (100 * ClipHigh(pXSprite->data3, kMaxPatrolSpotValue)) / kMaxPatrolSpotValue;
        //viewSetSystemMessage("%d / %d / %d / %d", hearChance, seeDist, seeChance, perc);

    }

    if (target >= 0) return target;
    else pXSprite->data3 -= ClipLow(((100 * pXSprite->data3) / kMaxPatrolSpotValue) >> 2, 3);
    return -1;
}

void aiPatrolThink(DBloodActor*actor) {

    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);


    int nTarget = -1;
    if ((nTarget = aiPatrolSearchTargets(pSprite, pXSprite)) != -1) {
        aiPatrolStop(pSprite, nTarget, pXSprite->dudeAmbush);
        return;
    }
    
    int omarker = pXSprite->target;
    spritetype* pMarker = NULL; XSPRITE* pXMarker = NULL;
    if (spriRangeIsFine(omarker)) {
        pMarker = &sprite[omarker];
        pXMarker = &xsprite[sprite[omarker].extra];
    }

    DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[pSprite->type - kDudeBase];
    bool crouch = aiPatrolCrouching(pXSprite->aiState), uwater = spriteIsUnderwater(pSprite);
    bool isFinal = (spriRangeIsFine(omarker) && omarker == pXSprite->target && xsprite[sprite[omarker].extra].data2 < 0);

    if (pSprite->type == kDudeModernCustom && ((uwater && !canSwim(pSprite)) || !canWalk(pSprite))) aiPatrolStop(pSprite, -1);
    else if (omarker <= 0) aiPatrolSetMarker(pSprite, pXSprite);
    else if (aiPatrolWaiting(pXSprite->aiState)) {

        if (pXSprite->stateTimer > 0 || (pXMarker && pXMarker->data1 == pXMarker->data2))
            return;
        
        aiPatrolSetMarker(pSprite, pXSprite);
        if (isFinal) {
            aiPatrolStop(pSprite, -1);
            return;
        }

    } else if (aiPatrolMarkerReached(pSprite, pXSprite)) {

        pMarker = &sprite[omarker];
        pXMarker = &xsprite[sprite[omarker].extra];

        if (pMarker->owner == pSprite->index)
            pMarker->owner = -1;
        
        if (pMarker->flags > 0) {
            if ((pMarker->flags & kModernTypeFlag1) && (pMarker->flags & kModernTypeFlag2))
                crouch = !crouch;
            else if (pMarker->flags & kModernTypeFlag2)
                crouch = false;
            else if (pMarker->flags & kModernTypeFlag1 && aiCanCrouch(pSprite))
                crouch = true;
        }

        trTriggerSprite(omarker, pXMarker, kCmdToggle); // trigger it!

        if (pXMarker->waitTime > 0 || pXMarker->data1 == pXMarker->data2) {

            if (uwater) aiPatrolState(pSprite, kAiStatePatrolWaitW);
            else if (crouch) aiPatrolState(pSprite, kAiStatePatrolWaitC);
            else aiPatrolState(pSprite, kAiStatePatrolWaitL);
            
            xvel[pSprite->index] = yvel[pSprite->index] = 0;
            if (pExtra->flying) zvel[pSprite->index] = 0;
            pSprite->ang = sprite[pXMarker->reference].ang & 2047;
            if (pXMarker->waitTime)
                pXSprite->stateTimer = (pXMarker->waitTime * 120) / 10;
            
            return;

        } else {

            aiPatrolSetMarker(pSprite, pXSprite);

        }

        // final marker reached, just make enemy to be normal
        if (isFinal) {
            aiPatrolStop(pSprite, -1);
            return;
        }

    }
    
    if (uwater) aiPatrolState(pSprite, kAiStatePatrolMoveW);
    else if (crouch) aiPatrolState(pSprite, kAiStatePatrolMoveC);
    else aiPatrolState(pSprite, kAiStatePatrolMoveL);
    return;

}
// ------------------------------------------------

int listTx(XSPRITE* pXRedir, int tx) {
    if (txIsRanged(pXRedir)) {
        if (tx == -1) tx = pXRedir->data1;
        else if (tx < pXRedir->data4) tx++;
        else tx = -1;
    } else {
        if (tx == -1) {
            for (int i = 0; i <= 3; i++) {
                if ((tx = GetDataVal(&sprite[pXRedir->reference], i)) <= 0) continue;
                else return tx;
            }
        } else {
            int saved = tx; bool savedFound = false;
            for (int i = 0; i <= 3; i++) {
                tx = GetDataVal(&sprite[pXRedir->reference], i);
                if (savedFound && tx > 0) return tx;
                else if (tx != saved) continue;
                else savedFound = true;
            }
        }
        
        tx = -1;
    }

    return tx;
}

XSPRITE* evrIsRedirector(int nSprite) {
    if (spriRangeIsFine(nSprite)) {
        switch (sprite[nSprite].type) {
        case kModernRandomTX:
        case kModernSequentialTX:
            if (xspriRangeIsFine(sprite[nSprite].extra) && xsprite[sprite[nSprite].extra].command == kCmdLink
                && !xsprite[sprite[nSprite].extra].locked) return &xsprite[sprite[nSprite].extra];
            break;
        }
    }

    return NULL;
}

XSPRITE* evrListRedirectors(int objType, int objXIndex, XSPRITE* pXRedir, int* tx) {
    if (!gEventRedirectsUsed) return NULL;
    else if (pXRedir && (*tx = listTx(pXRedir, *tx)) != -1)
        return pXRedir;

    int id = 0;
    switch (objType) {
        case OBJ_SECTOR:
            if (!xsectRangeIsFine(objXIndex)) return NULL;
            id = xsector[objXIndex].txID;
            break;
        case OBJ_SPRITE:
            if (!xspriRangeIsFine(objXIndex)) return NULL;
            id = xsprite[objXIndex].txID;
            break;
        case OBJ_WALL:
            if (!xwallRangeIsFine(objXIndex)) return NULL;
            id = xwall[objXIndex].txID;
            break;
        default:
            return NULL;
    }

    int nIndex = (pXRedir) ? pXRedir->reference : -1; bool prevFound = false;
    for (int i = bucketHead[id]; i < bucketHead[id + 1]; i++) {
        if (rxBucket[i].type != OBJ_SPRITE) continue;
        XSPRITE* pXSpr = evrIsRedirector(rxBucket[i].index);
        if (!pXSpr) continue;
        else if (prevFound || nIndex == -1) { *tx = listTx(pXSpr, *tx); return pXSpr; }
        else if (nIndex != pXSpr->reference) continue;
        else prevFound = true;
    }

    *tx = -1;
    return NULL;
}

// this function checks if all TX objects have the same value
bool incDecGoalValueIsReached(XSPRITE* pXSprite) {
    
    if (pXSprite->data3 != pXSprite->sysData1) return false;
    char buffer[5]; sprintf(buffer, "%d", abs(pXSprite->data1)); int len = int(strlen(buffer)); int rx = -1;
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type == OBJ_SPRITE && evrIsRedirector(rxBucket[i].index)) continue;
        for (int a = 0; a < len; a++) {
            if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                return false;
        }
    }

    XSPRITE* pXRedir = NULL; // check redirected TX buckets
    while ((pXRedir = evrListRedirectors(OBJ_SPRITE, sprite[pXSprite->reference].extra, pXRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            for (int a = 0; a < len; a++) {
                if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                    return false;
            }
        }
    }
    
    return true;
}

void seqSpawnerOffSameTx(XSPRITE* pXSource) {

    for (int i = 0; i < kMaxXSprites; i++) {
        
        XSPRITE* pXSprite = &xsprite[i];
        if (pXSprite->reference != pXSource->reference && spriRangeIsFine(pXSprite->reference)) {
            if (sprite[pXSprite->reference].type != kModernSeqSpawner) continue;
            else if (pXSprite->txID == pXSource->txID && pXSprite->state == 1) {
                evKill(pXSprite->reference, OBJ_SPRITE);
                pXSprite->state = 0;
            }
        }
    }
}

// this function can be called via sending numbered command to TX kChannelModernEndLevelCustom
// it allows to set custom next level instead of taking it from INI file.
void levelEndLevelCustom(int nLevel) {

    gGameOptions.uGameFlags |= GF_AdvanceLevel;
    gNextLevel = FindMapByIndex(currentLevel->cluster, nLevel + 1);
}

void callbackUniMissileBurst(int nSprite) // 22
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    spritetype* pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;

    for (int i = 0; i < 8; i++)
    {
        spritetype* pBurst = actSpawnSprite(pSprite, 5);

        pBurst->type = pSprite->type;
        pBurst->shade = pSprite->shade;
        pBurst->picnum = pSprite->picnum;

        pBurst->cstat = pSprite->cstat;
        if ((pBurst->cstat & CSTAT_SPRITE_BLOCK)) {
            pBurst->cstat &= ~CSTAT_SPRITE_BLOCK; // we don't want missiles impact each other
            evPost(pBurst->index, 3, 100, kCallbackMissileSpriteBlock); // so set blocking flag a bit later
        }

        pBurst->pal = pSprite->pal;
        pBurst->clipdist = pSprite->clipdist / 4;
        pBurst->flags = pSprite->flags;
        pBurst->xrepeat = pSprite->xrepeat / 2;
        pBurst->yrepeat = pSprite->yrepeat / 2;
        pBurst->ang = ((pSprite->ang + missileInfo[pSprite->type - kMissileBase].angleOfs) & 2047);
        pBurst->owner = pSprite->owner;

        actBuildMissile(pBurst, pBurst->extra, pSprite->index);

        int nAngle2 = (i << 11) / 8;
        int dx = 0;
        int dy = mulscale30r(nRadius, Sin(nAngle2));
        int dz = mulscale30r(nRadius, -Cos(nAngle2));
        if (i & 1)
        {
            dy >>= 1;
            dz >>= 1;
        }
        RotateVector(&dx, &dy, nAngle);
        xvel[pBurst->index] += dx;
        yvel[pBurst->index] += dy;
        zvel[pBurst->index] += dz;
        evPost(pBurst->index, 3, 960, kCallbackRemove);
    }
    evPost(nSprite, 3, 0, kCallbackRemove);
}


void callbackMakeMissileBlocking(int nSprite) // 23
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    sprite[nSprite].cstat |= CSTAT_SPRITE_BLOCK;
}

void callbackGenDudeUpdate(int nSprite) // 24
{
    if (spriRangeIsFine(nSprite))
        genDudeUpdate(&sprite[nSprite]);
}



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, GENDUDEEXTRA& w, GENDUDEEXTRA* def)
{
    if (arc.BeginObject(keyname))
    {
        arc.Array("initvals", w.initVals, 3)
            .Array("availdeaths", w.availDeaths, kDamageMax)
            ("movespeed", w.moveSpeed)
            ("firedist", w.fireDist)
            ("throwdist", w.throwDist)
            ("curweapon", w.curWeapon)
            ("weapontype", w.weaponType)
            ("basedispersion", w.baseDispersion)
            ("slavecount", w.slaveCount)
            ("lifeleech", w.nLifeLeech)
            .Array("slaves", w.slave, w.slaveCount)
            .Array("dmgcontrol", w.dmgControl, kDamageMax)
            .Array("updreq", w.updReq, kGenDudePropertyMax)
            ("flags", w.flags)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, SPRITEMASS& w, SPRITEMASS* def)
{
    static SPRITEMASS nul;
    if (arc.isReading()) w = {};
    if (arc.BeginObject(keyname))
    {
        arc ("seq", w.seqId, &nul.seqId)
            ("picnum", w.picnum, &nul.picnum)
            ("xrepeat", w.xrepeat, &nul.xrepeat)
            ("yrepeat", w.yrepeat, &nul.yrepeat)
            ("clipdist", w.clipdist)
            ("mass", w.mass)
            ("airvel", w.airVel)
            ("fraction", w.fraction)
            .EndObject();
    }
    return arc;
}

void SerializeNNExts(FSerializer& arc)
{
    if (arc.BeginObject("nnexts"))
    {
        // the GenDudeArray only contains valid info for kDudeModernCustom and kDudeModernCustomBurning so only save the relevant entries as these are not small.
        bool foundsome = false;
        for (int i = 0; i < kMaxSprites; i++)
        {
            if (activeSprites[i] && (sprite[i].type == kDudeModernCustom || sprite[i].type == kDudeModernCustomBurning))
            {
                if (!foundsome) arc.BeginArray("gendudeextra");
                foundsome = true;
                arc(nullptr, gGenDudeExtra[i]);
            }
        }
        if (foundsome) arc.EndArray();
        arc.SparseArray("spritemass", gSpriteMass, kMaxSprites, activeXSprites)
            ("proxyspritescount", gProxySpritesCount)
            .Array("proxyspriteslist", gProxySpritesList, gProxySpritesCount)
            ("sightspritescount", gSightSpritesCount)
            .Array("sightspriteslist", gSightSpritesList, gSightSpritesCount)
            ("physspritescount", gPhysSpritesCount)
            .Array("physspriteslist", gPhysSpritesList, gPhysSpritesCount)
            ("impactspritescount", gImpactSpritesCount)
            .Array("impactspriteslist", gImpactSpritesList, gImpactSpritesCount)
            ("eventredirects", gEventRedirectsUsed)
            .EndObject();
    }
}

///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////
END_BLD_NS

#endif
