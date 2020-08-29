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

#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "build.h"
#include "mmulti.h"
#include "actor.h"
#include "blood.h"
#include "callback.h"
#include "controls.h"
#include "eventq.h"
#include "fx.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "map2d.h"
#include "network.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "triggers.h"
#include "view.h"
#include "common_game.h"
#include "messages.h"
#include "nnexts.h"
#include "gstrings.h"

BEGIN_BLD_NS

extern bool gameRestart;

PROFILE gProfile[kMaxPlayers];

PLAYER gPlayer[kMaxPlayers];
PLAYER *gMe, *gView;

bool gBlueFlagDropped = false;
bool gRedFlagDropped = false;

// V = has effect in game, X = no effect in game
POWERUPINFO gPowerUpInfo[kMaxPowerUps] = {
    { -1, 1, 1, 1 },            // 00: V keys
    { -1, 1, 1, 1 },            // 01: V keys
    { -1, 1, 1, 1 },            // 02: V keys
    { -1, 1, 1, 1 },            // 03: V keys
    { -1, 1, 1, 1 },            // 04: V keys
    { -1, 1, 1, 1 },            // 05: V keys
    { -1, 1, 1, 1 },            // 06: V keys
    { -1, 0, 100, 100 },        // 07: V doctor's bag
    { -1, 0, 50, 100 },         // 08: V medicine pouch
    { -1, 0, 20, 100 },         // 09: V life essense
    { -1, 0, 100, 200 },        // 10: V life seed
    { -1, 0, 2, 200 },          // 11: V red potion
    { 783, 0, 3600, 432000 },   // 12: V feather fall
    { 896, 0, 3600, 432000 },   // 13: V cloak of invisibility
    { 825, 1, 3600, 432000 },   // 14: V death mask (invulnerability)
    { 827, 0, 3600, 432000 },   // 15: V jump boots
    { 828, 0, 3600, 432000 },   // 16: X raven flight
    { 829, 0, 3600, 1728000 },  // 17: V guns akimbo
    { 830, 0, 3600, 432000 },   // 18: V diving suit
    { 831, 0, 3600, 432000 },   // 19: V gas mask
    { -1, 0, 3600, 432000 },    // 20: X clone
    { 2566, 0, 3600, 432000 },  // 21: V crystal ball
    { 836, 0, 3600, 432000 },   // 22: X decoy
    { 853, 0, 3600, 432000 },   // 23: V doppleganger
    { 2428, 0, 3600, 432000 },  // 24: V reflective shots
    { 839, 0, 3600, 432000 },   // 25: V beast vision
    { 768, 0, 3600, 432000 },   // 26: X cloak of shadow (useless)
    { 840, 0, 3600, 432000 },   // 27: X rage shroom
    { 841, 0, 900, 432000 },    // 28: V delirium shroom
    { 842, 0, 3600, 432000 },   // 29: V grow shroom (gModernMap only)
    { 843, 0, 3600, 432000 },   // 30: V shrink shroom (gModernMap only)
    { -1, 0, 3600, 432000 },    // 31: X death mask (useless)
    { -1, 0, 3600, 432000 },    // 32: X wine goblet
    { -1, 0, 3600, 432000 },    // 33: X wine bottle
    { -1, 0, 3600, 432000 },    // 34: X skull grail
    { -1, 0, 3600, 432000 },    // 35: X silver grail
    { -1, 0, 3600, 432000 },    // 36: X tome
    { -1, 0, 3600, 432000 },    // 37: X black chest
    { -1, 0, 3600, 432000 },    // 38: X wooden chest
    { 837, 1, 3600, 432000 },   // 39: V asbestos armor
    { -1, 0, 1, 432000 },       // 40: V basic armor
    { -1, 0, 1, 432000 },       // 41: V body armor
    { -1, 0, 1, 432000 },       // 42: V fire armor
    { -1, 0, 1, 432000 },       // 43: V spirit armor
    { -1, 0, 1, 432000 },       // 44: V super armor
    { 0, 0, 0, 0 },             // 45: ? unknown
    { 0, 0, 0, 0 },             // 46: ? unknown
    { 0, 0, 0, 0 },             // 47: ? unknown
    { 0, 0, 0, 0 },             // 48: ? unknown
    { 0, 0, 0, 0 },             // 49: X dummy
    { 833, 1, 1, 1 }            // 50: V kModernItemLevelMap (gModernMap only)
};

int Handicap[] = {
    144, 208, 256, 304, 368
};

POSTURE gPostureDefaults[kModeMax][kPostureMax] = {
    
    // normal human
    {
        { 0x4000, 0x4000, 0x4000, 14, 17, 24, 16, 32, 80, 0x1600, 0x1200, 0xc00, 0x90, -0xbaaaa, -0x175555 },
        { 0x1200, 0x1200, 0x1200, 14, 17, 24, 16, 32, 80, 0x1400, 0x1000, -0x600, 0xb0, 0x5b05, 0 },
        { 0x2000, 0x2000, 0x2000, 22, 28, 24, 16, 16, 40, 0x800, 0x600, -0x600, 0xb0, 0, 0 },
    },

    // normal beast
    {
        { 0x4000, 0x4000, 0x4000, 14, 17, 24, 16, 32, 80, 0x1600, 0x1200, 0xc00, 0x90, -0xbaaaa, -0x175555 },
        { 0x1200, 0x1200, 0x1200, 14, 17, 24, 16, 32, 80, 0x1400, 0x1000, -0x600, 0xb0, 0x5b05, 0 },
        { 0x2000, 0x2000, 0x2000, 22, 28, 24, 16, 16, 40, 0x800, 0x600, -0x600, 0xb0, 0, 0 },
    },

    // shrink human
    {
        { 10384, 10384, 10384, 14, 17, 24, 16, 32, 80, 5632, 4608, 3072, 144, -564586, -1329173 },
        { 2108, 2108, 2108, 14, 17, 24, 16, 32, 80, 5120, 4096, -1536, 176, 0x5b05, 0 },
        { 2192, 2192, 2192, 22, 28, 24, 16, 16, 40, 2048, 1536, -1536, 176, 0, 0 },
    },

    // grown human
    {
        { 19384, 19384, 19384, 14, 17, 24, 16, 32, 80, 5632, 4608, 3072, 144, -1014586, -1779173 },
        { 5608, 5608, 5608, 14, 17, 24, 16, 32, 80, 5120, 4096, -1536, 176, 0x5b05, 0 },
        { 11192, 11192, 11192, 22, 28, 24, 16, 16, 40, 2048, 1536, -1536, 176, 0, 0 },
    },
};

AMMOINFO gAmmoInfo[] = {
    { 0, -1 },
    { 100, -1 },
    { 100, 4 },
    { 500, 5 },
    { 100, -1 },
    { 50, -1 },
    { 2880, -1 },
    { 250, -1 },
    { 100, -1 },
    { 100, -1 },
    { 50, -1 },
    { 50, -1 },
};

struct ARMORDATA {
    int at0;
    int at4;
    int at8;
    int atc;
    int at10;
    int at14;
};
ARMORDATA armorData[5] = {
    { 0x320, 0x640, 0x320, 0x640, 0x320, 0x640 },
    { 0x640, 0x640, 0, 0x640, 0, 0x640 },
    { 0, 0x640, 0x640, 0x640, 0, 0x640 },
    { 0, 0x640, 0, 0x640, 0x640, 0x640 },
    { 0xc80, 0xc80, 0xc80, 0xc80, 0xc80, 0xc80 }
};

void PlayerSurvive(int, int);
void PlayerKneelsOver(int, int);

int nPlayerSurviveClient = seqRegisterClient(PlayerSurvive);
int nPlayerKneelClient = seqRegisterClient(PlayerKneelsOver);

struct VICTORY {
    const char *at0;
    int at4;
};

VICTORY gVictory[] = {
    { "%s boned %s like a fish", 4100 },
    { "%s castrated %s", 4101 },
    { "%s creamed %s", 4102 },
    { "%s destroyed %s", 4103 },
    { "%s diced %s", 4104 },
    { "%s disemboweled %s", 4105 },
    { "%s flattened %s", 4106 },
    { "%s gave %s Anal Justice", 4107 },
    { "%s gave AnAl MaDnEsS to %s", 4108 },
    { "%s hurt %s real bad", 4109 },
    { "%s killed %s", 4110 },
    { "%s made mincemeat out of %s", 4111 },
    { "%s massacred %s", 4112 },
    { "%s mutilated %s", 4113 },
    { "%s reamed %s", 4114 },
    { "%s ripped %s a new orifice", 4115 },
    { "%s slaughtered %s", 4116 },
    { "%s sliced %s", 4117 },
    { "%s smashed %s", 4118 },
    { "%s sodomized %s", 4119 },
    { "%s splattered %s", 4120 },
    { "%s squashed %s", 4121 },
    { "%s throttled %s", 4122 },
    { "%s wasted %s", 4123 },
    { "%s body bagged %s", 4124 },
};

struct SUICIDE {
    const char *at0;
    int at4;
};

SUICIDE gSuicide[] = {
    { "%s is excrement", 4202 },
    { "%s is hamburger", 4203 },
    { "%s suffered scrotum separation", 4204 },
    { "%s volunteered for population control", 4206 },
    { "%s has suicided", 4207 },
};

struct DAMAGEINFO {
    int at0;
    int at4[3];
    int at10[3];
};

DAMAGEINFO damageInfo[7] = {
    { -1, 731, 732, 733, 710, 710, 710 },
    { 1, 742, 743, 744, 711, 711, 711 },
    { 0, 731, 732, 733, 712, 712, 712 },
    { 1, 731, 732, 733, 713, 713, 713 },
    { -1, 724, 724, 724, 714, 714, 714 },
    { 2, 731, 732, 733, 715, 715, 715 },
    { 0, 0, 0, 0, 0, 0, 0 }
};

int powerupCheck(PLAYER *pPlayer, int nPowerUp)
{
    dassert(pPlayer != NULL);
    dassert(nPowerUp >= 0 && nPowerUp < kMaxPowerUps);
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0 && !packItemActive(pPlayer, nPack))
        return 0;
    return pPlayer->pwUpTime[nPowerUp];
}


char powerupActivate(PLAYER *pPlayer, int nPowerUp)
{
    if (powerupCheck(pPlayer, nPowerUp) > 0 && gPowerUpInfo[nPowerUp].pickupOnce)
        return 0;
    if (!pPlayer->pwUpTime[nPowerUp])
        pPlayer->pwUpTime[nPowerUp] = gPowerUpInfo[nPowerUp].bonusTime;
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0)
        pPlayer->packSlots[nPack].isActive = 1;
    
    switch (nPowerUp + kItemBase) {
        #ifdef NOONE_EXTENSIONS
        case kItemModernMapLevel:
            if (gModernMap) gFullMap = true;
            break;
        case kItemShroomShrink:
            if (!gModernMap) break;
            else if (isGrown(pPlayer->pSprite)) playerDeactivateShrooms(pPlayer);
            else playerSizeShrink(pPlayer, 2);
            break;
        case kItemShroomGrow:
            if (!gModernMap) break;
            else if (isShrinked(pPlayer->pSprite)) playerDeactivateShrooms(pPlayer);
            else {
                playerSizeGrow(pPlayer, 2);
                if (powerupCheck(&gPlayer[pPlayer->pSprite->type - kDudePlayer1], kPwUpShadowCloak) > 0) {
                    powerupDeactivate(pPlayer, kPwUpShadowCloak);
                    pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
                }

                if (ceilIsTooLow(pPlayer->pSprite))
                    actDamageSprite(pPlayer->pSprite->index, pPlayer->pSprite, DAMAGE_TYPE_3, 65535);
            }
            break;
        #endif
        case kItemFeatherFall:
        case kItemJumpBoots:
            pPlayer->damageControl[0]++;
            break;
        case kItemReflectShots: // reflective shots
            if (pPlayer == gMe && gGameOptions.nGameType == 0)
                sfxSetReverb2(1);
            break;
        case kItemDeathMask:
            for (int i = 0; i < 7; i++)
                pPlayer->damageControl[i]++;
            break;
        case kItemDivingSuit: // diving suit
            pPlayer->damageControl[4]++;
            if (pPlayer == gMe && gGameOptions.nGameType == 0)
                sfxSetReverb(1);
            break;
        case kItemGasMask:
            pPlayer->damageControl[4]++;
            break;
        case kItemArmorAsbest:
            pPlayer->damageControl[1]++;
            break;
        case kItemTwoGuns:
            pPlayer->newWeapon = pPlayer->curWeapon;
            WeaponRaise(pPlayer);
            break;
    }
    sfxPlay3DSound(pPlayer->pSprite, 776, -1, 0);
    return 1;
}

void powerupDeactivate(PLAYER *pPlayer, int nPowerUp)
{
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0)
        pPlayer->packSlots[nPack].isActive = 0;
    
    switch (nPowerUp + kItemBase) {
        #ifdef NOONE_EXTENSIONS
        case kItemShroomShrink:
            if (gModernMap) {
                playerSizeReset(pPlayer);
                if (ceilIsTooLow(pPlayer->pSprite))
                    actDamageSprite(pPlayer->pSprite->index, pPlayer->pSprite, DAMAGE_TYPE_3, 65535);
            }
            break;
        case kItemShroomGrow:
            if (gModernMap) playerSizeReset(pPlayer);
            break;
        #endif
        case kItemFeatherFall:
        case kItemJumpBoots:
            pPlayer->damageControl[0]--;
            break;
        case kItemDeathMask:
            for (int i = 0; i < 7; i++)
                pPlayer->damageControl[i]--;
            break;
        case kItemDivingSuit:
            pPlayer->damageControl[4]--;
            if (pPlayer == gMe && VanillaMode() ? true : pPlayer->pwUpTime[24] == 0)
                sfxSetReverb(0);
            break;
        case kItemReflectShots:
            if (pPlayer == gMe && VanillaMode() ? true : pPlayer->packSlots[1].isActive == 0)
                sfxSetReverb(0);
            break;
        case kItemGasMask:
            pPlayer->damageControl[4]--;
            break;
        case kItemArmorAsbest:
            pPlayer->damageControl[1]--;
            break;
        case kItemTwoGuns:
            pPlayer->newWeapon = pPlayer->curWeapon;
            WeaponRaise(pPlayer);
            break;
    }
}

void powerupSetState(PLAYER *pPlayer, int nPowerUp, char bState)
{
    if (!bState)
        powerupActivate(pPlayer, nPowerUp);
    else
        powerupDeactivate(pPlayer, nPowerUp);
}

void powerupProcess(PLAYER *pPlayer)
{
    pPlayer->packItemTime = ClipLow(pPlayer->packItemTime-4, 0);
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        int nPack = powerupToPackItem(i);
        if (nPack >= 0)
        {
            if (pPlayer->packSlots[nPack].isActive)
            {
                pPlayer->pwUpTime[i] = ClipLow(pPlayer->pwUpTime[i]-4, 0);
                if (pPlayer->pwUpTime[i])
                    pPlayer->packSlots[nPack].curAmount = (100*pPlayer->pwUpTime[i])/gPowerUpInfo[i].bonusTime;
                else
                {
                    powerupDeactivate(pPlayer, i);
                    if (pPlayer->packItemId == nPack)
                        pPlayer->packItemId = 0;
                }
            }
        }
        else if (pPlayer->pwUpTime[i] > 0)
        {
            pPlayer->pwUpTime[i] = ClipLow(pPlayer->pwUpTime[i]-4, 0);
            if (!pPlayer->pwUpTime[i])
                powerupDeactivate(pPlayer, i);
        }
    }
}

void powerupClear(PLAYER *pPlayer)
{
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        pPlayer->pwUpTime[i] = 0;
    }
}

void powerupInit(void)
{
}

int packItemToPowerup(int nPack)
{
    int nPowerUp = -1;
    switch (nPack) {
        case 0:
            break;
        case 1:
            nPowerUp = kPwUpDivingSuit;
            break;
        case 2:
            nPowerUp = kPwUpCrystalBall;
            break;
        case 3:
            nPowerUp = kPwUpBeastVision;
            break;
        case 4:
            nPowerUp = kPwUpJumpBoots;
            break;
        default:
            ThrowError("Unhandled pack item %d", nPack);
            break;
    }
    return nPowerUp;
}

int powerupToPackItem(int nPowerUp)
{
    switch (nPowerUp) {
        case kPwUpDivingSuit:
            return 1;
        case kPwUpCrystalBall:
            return 2;
        case kPwUpBeastVision:
            return 3;
        case kPwUpJumpBoots:
            return 4;
    }
    return -1;
}

char packAddItem(PLAYER *pPlayer, unsigned int nPack)
{
    if (nPack <= 4)
    {
        if (pPlayer->packSlots[nPack].curAmount >= 100)
            return 0;
        pPlayer->packSlots[nPack].curAmount = 100;
        int nPowerUp = packItemToPowerup(nPack);
        if (nPowerUp >= 0)
            pPlayer->pwUpTime[nPowerUp] = gPowerUpInfo[nPowerUp].bonusTime;
        if (pPlayer->packItemId == -1)
            pPlayer->packItemId = nPack;
        if (!pPlayer->packSlots[pPlayer->packItemId].curAmount)
            pPlayer->packItemId = nPack;
    }
    else
        ThrowError("Unhandled pack item %d", nPack);
    return 1;
}

int packCheckItem(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packSlots[nPack].curAmount;
}

char packItemActive(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packSlots[nPack].isActive;
}

void packUseItem(PLAYER *pPlayer, int nPack)
{
    char v4 = 0;
    int nPowerUp = -1;
    if (pPlayer->packSlots[nPack].curAmount > 0)
    {
        switch (nPack)
        {
        case 0:
        {
            XSPRITE *pXSprite = pPlayer->pXSprite;
            unsigned int health = pXSprite->health>>4;
            if (health < 100)
            {
                int heal = ClipHigh(100-health, pPlayer->packSlots[0].curAmount);
                actHealDude(pXSprite, heal, 100);
                pPlayer->packSlots[0].curAmount -= heal;
            }
            break;
        }
        case 1:
            v4 = 1;
            nPowerUp = kPwUpDivingSuit;
            break;
        case 2:
            v4 = 1;
            nPowerUp = kPwUpCrystalBall;
            break;
        case 3:
            v4 = 1;
            nPowerUp = kPwUpBeastVision;
            break;
        case 4:
            v4 = 1;
            nPowerUp = kPwUpJumpBoots;
            break;
        default:
            ThrowError("Unhandled pack item %d", nPack);
            return;
        }
    }
    pPlayer->packItemTime = 0;
    if (v4)
        powerupSetState(pPlayer, nPowerUp, pPlayer->packSlots[nPack].isActive);
}

void packPrevItem(PLAYER *pPlayer)
{
    if (pPlayer->packItemTime > 0)
    {
        for (int i = 0; i < 2; i++)
        {
            for (int nPrev = pPlayer->packItemId-1; nPrev >= 0; nPrev--)
            {
                if (pPlayer->packSlots[nPrev].curAmount)
                {
                    pPlayer->packItemId = nPrev;
                    pPlayer->packItemTime = 600;
                    return;
                }
            }
            pPlayer->packItemId = 4;
            if (pPlayer->packSlots[4].curAmount) break;
        }
    }
    
    pPlayer->packItemTime = 600;
}

void packNextItem(PLAYER* pPlayer)
{
    if (pPlayer->packItemTime > 0)
    {
        for (int i = 0; i < 2; i++)
        {
            for (int nNext = pPlayer->packItemId + 1; nNext < 5; nNext++)
            {
                if (pPlayer->packSlots[nNext].curAmount)
                {
                    pPlayer->packItemId = nNext;
                    pPlayer->packItemTime = 600;
                    return;
                }
            }
            pPlayer->packItemId = 0;
            if (pPlayer->packSlots[0].curAmount) break;
        }
    }
    pPlayer->packItemTime = 600;
}

char playerSeqPlaying(PLAYER * pPlayer, int nSeq)
{
    int nCurSeq = seqGetID(3, pPlayer->pSprite->extra);
    if (pPlayer->pDudeInfo->seqStartID+nSeq == nCurSeq && seqGetStatus(3,pPlayer->pSprite->extra) >= 0)
        return 1;
    return 0;
}

void playerSetRace(PLAYER *pPlayer, int nLifeMode)
{
    dassert(nLifeMode >= kModeHuman && nLifeMode <= kModeHumanGrown);
    DUDEINFO *pDudeInfo = pPlayer->pDudeInfo;
    *pDudeInfo = gPlayerTemplate[nLifeMode];
    pPlayer->lifeMode = nLifeMode;
    
    // By NoOne: don't forget to change clipdist for grow and shrink modes
    pPlayer->pSprite->clipdist = pDudeInfo->clipdist;
    
    for (int i = 0; i < 7; i++)
        pDudeInfo->at70[i] = mulscale8(Handicap[gProfile[pPlayer->nPlayer].skill], pDudeInfo->startDamage[i]);
}

void playerSetGodMode(PLAYER *pPlayer, char bGodMode)
{
    if (bGodMode)
    {
        for (int i = 0; i < 7; i++)
            pPlayer->damageControl[i]++;
    }
    else
    {
        for (int i = 0; i < 7; i++)
            pPlayer->damageControl[i]--;
    }
    pPlayer->godMode = bGodMode;
}

void playerResetInertia(PLAYER *pPlayer)
{
    POSTURE *pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
    pPlayer->zView = pPlayer->pSprite->z-pPosture->eyeAboveZ;
    pPlayer->zWeapon = pPlayer->pSprite->z-pPosture->weaponAboveZ;
    viewBackupView(pPlayer->nPlayer);
}

void playerCorrectInertia(PLAYER* pPlayer, vec3_t const *oldpos)
{
    pPlayer->zView += pPlayer->pSprite->z-oldpos->z;
    pPlayer->zWeapon += pPlayer->pSprite->z-oldpos->z;
    viewCorrectViewOffsets(pPlayer->nPlayer, oldpos);
}

void playerResetPowerUps(PLAYER* pPlayer)
{
    for (int i = 0; i < kMaxPowerUps; i++) {
        if (!VanillaMode() && (i == kPwUpJumpBoots || i == kPwUpDivingSuit || i == kPwUpCrystalBall || i == kPwUpBeastVision))
            continue;
        pPlayer->pwUpTime[i] = 0;
    }
}

void playerResetPosture(PLAYER* pPlayer) {
    memcpy(pPlayer->pPosture, gPostureDefaults, sizeof(gPostureDefaults));
}

void playerStart(int nPlayer, int bNewLevel)
{
    PLAYER* pPlayer = &gPlayer[nPlayer];
    InputPacket* pInput = &pPlayer->input;
    ZONE* pStartZone = NULL;

    // normal start position
    if (gGameOptions.nGameType <= 1)
        pStartZone = &gStartZone[nPlayer];
    
    #ifdef NOONE_EXTENSIONS
    // let's check if there is positions of teams is specified
    // if no, pick position randomly, just like it works in vanilla.
    else if (gModernMap && gGameOptions.nGameType == 3 && gTeamsSpawnUsed == true) {
        int maxRetries = 5;
        while (maxRetries-- > 0) {
            if (pPlayer->teamId == 0) pStartZone = &gStartZoneTeam1[Random(3)];
            else pStartZone = &gStartZoneTeam2[Random(3)];

            if (maxRetries != 0) {
                // check if there is no spawned player in selected zone
                for (int i = headspritesect[pStartZone->sectnum]; i >= 0; i = nextspritesect[i]) {
                    spritetype* pSprite = &sprite[i];
                    if (pStartZone->x == pSprite->x && pStartZone->y == pSprite->y && IsPlayerSprite(pSprite)) {
                        pStartZone = NULL;
                        break;
                    }
                }
            }

            if (pStartZone != NULL)
                break;
        }
    
    }
    #endif
    else {
        pStartZone = &gStartZone[Random(8)];
    }

    spritetype *pSprite = actSpawnSprite(pStartZone->sectnum, pStartZone->x, pStartZone->y, pStartZone->z, 6, 1);
    dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    pPlayer->pSprite = pSprite;
    pPlayer->pXSprite = pXSprite;
    pPlayer->nSprite = pSprite->index;
    DUDEINFO *pDudeInfo = &dudeInfo[kDudePlayer1 + nPlayer - kDudeBase];
    pPlayer->pDudeInfo = pDudeInfo;
    playerSetRace(pPlayer, kModeHuman);
    playerResetPosture(pPlayer);
    seqSpawn(pDudeInfo->seqStartID, 3, pSprite->extra, -1);
    if (pPlayer == gMe)
        SetBitString(show2dsprite, pSprite->index);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z -= bottom - pSprite->z;
    pSprite->pal = 11+(pPlayer->teamId&3);
    pPlayer->angold = pSprite->ang = pStartZone->ang;
    pPlayer->q16ang = fix16_from_int(pSprite->ang);
    pSprite->type = kDudePlayer1+nPlayer;
    pSprite->clipdist = pDudeInfo->clipdist;
    pSprite->flags = 15;
    pXSprite->burnTime = 0;
    pXSprite->burnSource = -1;
    pPlayer->pXSprite->health = pDudeInfo->startHealth<<4;
    pPlayer->pSprite->cstat &= (unsigned short)~32768;
    pPlayer->bloodlust = 0;
    pPlayer->q16horiz = 0;
    pPlayer->q16slopehoriz = 0;
    pPlayer->q16look = 0;
    pPlayer->slope = 0;
    pPlayer->fraggerId = -1;
    pPlayer->underwaterTime = 1200;
    pPlayer->bloodTime = 0;
    pPlayer->gooTime = 0;
    pPlayer->wetTime = 0;
    pPlayer->bubbleTime = 0;
    pPlayer->at306 = 0;
    pPlayer->restTime = 0;
    pPlayer->kickPower = 0;
    pPlayer->laughCount = 0;
    pPlayer->spin = 0;
    pPlayer->posture = 0;
    pPlayer->voodooTarget = -1;
    pPlayer->voodooTargets = 0;
    pPlayer->voodooVar1 = 0;
    pPlayer->vodooVar2 = 0;
    playerResetInertia(pPlayer);
    pPlayer->zWeaponVel = 0;
    pPlayer->relAim.dx = 0x4000;
    pPlayer->relAim.dy = 0;
    pPlayer->relAim.dz = 0;
    pPlayer->aimTarget = -1;
    pPlayer->zViewVel = pPlayer->zWeaponVel;
    if (!(gGameOptions.nGameType == 1 && gGameOptions.bKeepKeysOnRespawn && !bNewLevel))
        for (int i = 0; i < 8; i++)
            pPlayer->hasKey[i] = gGameOptions.nGameType >= 2;
    pPlayer->hasFlag = 0;
    for (int i = 0; i < 8; i++)
        pPlayer->used2[i] = -1;
    for (int i = 0; i < 7; i++)
        pPlayer->damageControl[i] = 0;
    if (pPlayer->godMode)
        playerSetGodMode(pPlayer, 1);
    gInfiniteAmmo = 0;
    gFullMap = 0;
    pPlayer->throwPower = 0;
    pPlayer->deathTime = 0;
    pPlayer->nextWeapon = 0;
    xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
    pInput->q16avel = 0;
    pInput->actions = 0;
    pInput->fvel = 0;
    pInput->svel = 0;
    pInput->q16horz = 0;
    pPlayer->flickerEffect = 0;
    pPlayer->quakeEffect = 0;
    pPlayer->tiltEffect = 0;
    pPlayer->visibility = 0;
    pPlayer->painEffect = 0;
    pPlayer->blindEffect = 0;
    pPlayer->chokeEffect = 0;
    pPlayer->handTime = 0;
    pPlayer->weaponTimer = 0;
    pPlayer->weaponState = 0;
    pPlayer->weaponQav = -1;
    #ifdef NOONE_EXTENSIONS
    playerQavSceneReset(pPlayer); // reset qav scene
    
    // assign or update player's sprite index for conditions
    if (gModernMap) {

        for (int nSprite = headspritestat[kStatModernPlayerLinker]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
            XSPRITE* pXCtrl = &xsprite[sprite[nSprite].extra];
            if (pXCtrl->data1 == pPlayer->nPlayer + 1) {
                int nSpriteOld = pXCtrl->sysData1;
                trPlayerCtrlLink(pXCtrl, pPlayer, (nSpriteOld < 0) ? true : false);
                if (nSpriteOld > 0)
                    condUpdateObjectIndex(OBJ_SPRITE, nSpriteOld, pXCtrl->sysData1);
            }
        }

    }

    #endif
    pPlayer->hand = 0;
    pPlayer->nWaterPal = 0;
    playerResetPowerUps(pPlayer);

    if (pPlayer == gMe)
    {
        viewInitializePrediction();
        gViewLook = pPlayer->q16look;
        gViewAngle = pPlayer->q16ang;
        gViewMap.x = pPlayer->pSprite->x;
        gViewMap.y = pPlayer->pSprite->y;
        gViewMap.angle = pPlayer->pSprite->ang;
    }
    if (IsUnderwaterSector(pSprite->sectnum))
    {
        pPlayer->posture = 1;
        pPlayer->pXSprite->medium = kMediumWater;
    }
}

void playerReset(PLAYER *pPlayer)
{
    static int dword_136400[] = {
        3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
    };
    static int dword_136438[] = {
        3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
    };
    dassert(pPlayer != NULL);
    for (int i = 0; i < 14; i++)
    {
        pPlayer->hasWeapon[i] = gInfiniteAmmo;
        pPlayer->weaponMode[i] = 0;
    }
    pPlayer->hasWeapon[1] = 1;
    pPlayer->curWeapon = 0;
    pPlayer->qavCallback = -1;
    pPlayer->newWeapon = 1;
    for (int i = 0; i < 14; i++)
    {
        pPlayer->weaponOrder[0][i] = dword_136400[i];
        pPlayer->weaponOrder[1][i] = dword_136438[i];
    }
    for (int i = 0; i < 12; i++)
    {
        if (gInfiniteAmmo)
            pPlayer->ammoCount[i] = gAmmoInfo[i].max;
        else
            pPlayer->ammoCount[i] = 0;
    }
    for (int i = 0; i < 3; i++)
        pPlayer->armor[i] = 0;
    pPlayer->weaponTimer = 0;
    pPlayer->weaponState = 0;
    pPlayer->weaponQav = -1;
    pPlayer->qavLoop = 0;
    pPlayer->packItemId = -1;

    for (int i = 0; i < 5; i++) {
        pPlayer->packSlots[i].isActive = 0;
        pPlayer->packSlots[i].curAmount = 0;
    }
    #ifdef NOONE_EXTENSIONS
    playerQavSceneReset(pPlayer);
    #endif
    // reset posture (mainly required for resetting movement speed and jump height)
    playerResetPosture(pPlayer);

}

int dword_21EFB0[8];
int dword_21EFD0[8];

void playerInit(int nPlayer, unsigned int a2)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    if (!(a2&1))
        memset(pPlayer, 0, sizeof(PLAYER));
    pPlayer->nPlayer = nPlayer;
    pPlayer->teamId = nPlayer;
    if (gGameOptions.nGameType == 3)
        pPlayer->teamId = nPlayer&1;
    pPlayer->fragCount = 0;
    memset(dword_21EFB0, 0, sizeof(dword_21EFB0));
    memset(dword_21EFD0, 0, sizeof(dword_21EFD0));
    memset(pPlayer->fragInfo, 0, sizeof(pPlayer->fragInfo));

    if (!(a2&1))
        playerReset(pPlayer);
}

char sub_3A158(PLAYER *a1, spritetype *a2)
{
    for (int nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        if (a2 && a2->index == nSprite)
            continue;
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->type == kThingDroppedLifeLeech && actOwnerIdToSpriteId(pSprite->owner) == a1->nSprite)
            return 1;
    }
    return 0;
}

char PickupItem(PLAYER *pPlayer, spritetype *pItem) {
    
    spritetype *pSprite = pPlayer->pSprite; XSPRITE *pXSprite = pPlayer->pXSprite;
    char buffer[80]; int pickupSnd = 775; int nType = pItem->type - kItemBase;

    switch (pItem->type) {
        case kItemShadowCloak:
            #ifdef NOONE_EXTENSIONS
            if (isGrown(pPlayer->pSprite) || !powerupActivate(pPlayer, nType)) return false;
            #else
            if (!powerupActivate(pPlayer, nType)) return false;
            #endif
            break;
        #ifdef NOONE_EXTENSIONS
        case kItemShroomShrink:
        case kItemShroomGrow:
            
            if (gModernMap) {
                switch (pItem->type) {
                    case kItemShroomShrink:
                        if (isShrinked(pSprite)) return false;
                        break;
                    case kItemShroomGrow:
                        if (isGrown(pSprite)) return false;
                        break;
                }

                powerupActivate(pPlayer, nType);
            }
            
            break;
        #endif
        case kItemFlagABase:
        case kItemFlagBBase: {
            if (gGameOptions.nGameType != 3 || pItem->extra <= 0) return 0;
            XSPRITE * pXItem = &xsprite[pItem->extra];
            if (pItem->type == kItemFlagABase) {
                if (pPlayer->teamId == 1) {
                    if ((pPlayer->hasFlag & 1) == 0 && pXItem->state) {
                        pPlayer->hasFlag |= 1;
                        pPlayer->used2[0] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, kCmdOff);
                        sprintf(buffer, "%s stole Blue Flag", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8007, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }

                if (pPlayer->teamId == 0) {

                    if ((pPlayer->hasFlag & 1) != 0 && !pXItem->state) {
                        pPlayer->hasFlag &= ~1;
                        pPlayer->used2[0] = -1;
                        trTriggerSprite(pItem->index, pXItem, kCmdOn);
                        sprintf(buffer, "%s returned Blue Flag", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8003, 255, 2, 0);
                        viewSetMessage(buffer);
                    }

                    if ((pPlayer->hasFlag & 2) != 0 && pXItem->state) {
                        pPlayer->hasFlag &= ~2;
                        pPlayer->used2[1] = -1;
                        dword_21EFB0[pPlayer->teamId] += 10;
                        dword_21EFD0[pPlayer->teamId] += 240;
                        evSend(0, 0, 81, kCmdOn);
                        sprintf(buffer, "%s captured Red Flag!", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8001, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }

            }
            else if (pItem->type == kItemFlagBBase) {

                if (pPlayer->teamId == 0) {
                    if ((pPlayer->hasFlag & 2) == 0 && pXItem->state) {
                        pPlayer->hasFlag |= 2;
                        pPlayer->used2[1] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, kCmdOff);
                        sprintf(buffer, "%s stole Red Flag", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8006, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }

                if (pPlayer->teamId == 1) {
                    if ((pPlayer->hasFlag & 2) != 0 && !pXItem->state)
                    {
                        pPlayer->hasFlag &= ~2;
                        pPlayer->used2[1] = -1;
                        trTriggerSprite(pItem->index, pXItem, kCmdOn);
                        sprintf(buffer, "%s returned Red Flag", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8002, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                    if ((pPlayer->hasFlag & 1) != 0 && pXItem->state)
                    {
                        pPlayer->hasFlag &= ~1;
                        pPlayer->used2[0] = -1;
                        dword_21EFB0[pPlayer->teamId] += 10;
                        dword_21EFD0[pPlayer->teamId] += 240;
                        evSend(0, 0, 80, kCmdOn);
                        sprintf(buffer, "%s captured Blue Flag!", gProfile[pPlayer->nPlayer].name);
                        sndStartSample(8000, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }
            }
        }
        return 0;
        case kItemFlagA:
            if (gGameOptions.nGameType != 3) return 0;
            evKill(pItem->index, 3, kCallbackReturnFlag);
            pPlayer->hasFlag |= 1;
            pPlayer->used2[0] = pItem->index;
            gBlueFlagDropped = false;
            break;
        case kItemFlagB:
            if (gGameOptions.nGameType != 3) return 0;
            evKill(pItem->index, 3, kCallbackReturnFlag);
            pPlayer->hasFlag |= 2;
            pPlayer->used2[1] = pItem->index;
            gRedFlagDropped = false;
            break;
        case kItemArmorBasic:
        case kItemArmorBody:
        case kItemArmorFire:
        case kItemArmorSpirit:
        case kItemArmorSuper: {
            ARMORDATA *pArmorData = &armorData[pItem->type - kItemArmorBasic]; bool pickedUp = false;
            if (pPlayer->armor[1] < pArmorData->atc) {
                pPlayer->armor[1] = ClipHigh(pPlayer->armor[1]+pArmorData->at8, pArmorData->atc);
                pickedUp = true;
            }
        
            if (pPlayer->armor[0] < pArmorData->at4) {
                pPlayer->armor[0] = ClipHigh(pPlayer->armor[0]+pArmorData->at0, pArmorData->at4);
                pickedUp = true;
            }

            if (pPlayer->armor[2] < pArmorData->at14) {
                pPlayer->armor[2] = ClipHigh(pPlayer->armor[2]+pArmorData->at10, pArmorData->at14);
                pickedUp = true;
            }
        
            if (!pickedUp) return 0;
            pickupSnd = 779;
            break;
        }
        case kItemCrystalBall:
            if (gGameOptions.nGameType == 0 || !packAddItem(pPlayer, gItemData[nType].packSlot)) return 0;
            break;
        case kItemKeySkull:
        case kItemKeyEye:
        case kItemKeyFire:
        case kItemKeyDagger:
        case kItemKeySpider:
        case kItemKeyMoon:
        case kItemKeyKey7:
            if (pPlayer->hasKey[pItem->type-99]) return 0;
            pPlayer->hasKey[pItem->type-99] = 1;
            pickupSnd = 781;
            break;
        case kItemHealthMedPouch:
        case kItemHealthLifeEssense:
        case kItemHealthLifeSeed:
        case kItemHealthRedPotion:  {
            int addPower = gPowerUpInfo[nType].bonusTime;
            #ifdef NOONE_EXTENSIONS
            // allow custom amount for item
            if (gModernMap && sprite[pItem->index].extra >= 0 && xsprite[sprite[pItem->index].extra].data1 > 0)
                addPower = xsprite[sprite[pItem->index].extra].data1;
            #endif
        
            if (!actHealDude(pXSprite, addPower, gPowerUpInfo[nType].maxTime)) return 0;
            return 1;
        }
        case kItemHealthDoctorBag:
        case kItemJumpBoots:
        case kItemDivingSuit:
        case kItemBeastVision:
            if (!packAddItem(pPlayer, gItemData[nType].packSlot)) return 0;
            break;
        default:
            if (!powerupActivate(pPlayer, nType)) return 0;
            return 1;
    }
    
    sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, pickupSnd, pSprite->sectnum);
    return 1;
}

char PickupAmmo(PLAYER* pPlayer, spritetype* pAmmo) {
    const AMMOITEMDATA* pAmmoItemData = &gAmmoItemData[pAmmo->type - kItemAmmoBase];
    int nAmmoType = pAmmoItemData->type;

    if (pPlayer->ammoCount[nAmmoType] >= gAmmoInfo[nAmmoType].max) return 0;
    #ifdef NOONE_EXTENSIONS
    else if (gModernMap && pAmmo->extra >= 0 && xsprite[pAmmo->extra].data1 > 0) // allow custom amount for item
        pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + xsprite[pAmmo->extra].data1, gAmmoInfo[nAmmoType].max);
    #endif
    else
        pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType]+pAmmoItemData->count, gAmmoInfo[nAmmoType].max);

    if (pAmmoItemData->weaponType)  pPlayer->hasWeapon[pAmmoItemData->weaponType] = 1;
    sfxPlay3DSound(pPlayer->pSprite, 782, -1, 0);
    return 1;
}

char PickupWeapon(PLAYER *pPlayer, spritetype *pWeapon) {
    const WEAPONITEMDATA *pWeaponItemData = &gWeaponItemData[pWeapon->type - kItemWeaponBase];
    int nWeaponType = pWeaponItemData->type;
    int nAmmoType = pWeaponItemData->ammoType;
    if (!pPlayer->hasWeapon[nWeaponType] || gGameOptions.nWeaponSettings == 2 || gGameOptions.nWeaponSettings == 3) {
        if (pWeapon->type == kItemWeaponLifeLeech && gGameOptions.nGameType > 1 && sub_3A158(pPlayer, NULL))
            return 0;
        pPlayer->hasWeapon[nWeaponType] = 1;
        if (nAmmoType == -1) return 0;
        // allow to set custom ammo count for weapon pickups
        #ifdef NOONE_EXTENSIONS
        else if (gModernMap && pWeapon->extra >= 0 && xsprite[pWeapon->extra].data1 > 0)
            pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + xsprite[pWeapon->extra].data1, gAmmoInfo[nAmmoType].max);
        #endif
        else
            pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponItemData->count, gAmmoInfo[nAmmoType].max);

        int nNewWeapon = WeaponUpgrade(pPlayer, nWeaponType);
        if (nNewWeapon != pPlayer->curWeapon) {
            pPlayer->weaponState = 0;
            pPlayer->nextWeapon = nNewWeapon;
        }
        sfxPlay3DSound(pPlayer->pSprite, 777, -1, 0);
        return 1;
    }
    
    if (!actGetRespawnTime(pWeapon) || nAmmoType == -1 || pPlayer->ammoCount[nAmmoType] >= gAmmoInfo[nAmmoType].max) return 0;    
    #ifdef NOONE_EXTENSIONS
        else if (gModernMap && pWeapon->extra >= 0 && xsprite[pWeapon->extra].data1 > 0)
            pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + xsprite[pWeapon->extra].data1, gAmmoInfo[nAmmoType].max);
    #endif
    else
        pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType]+pWeaponItemData->count, gAmmoInfo[nAmmoType].max);

    sfxPlay3DSound(pPlayer->pSprite, 777, -1, 0);
    return 1;
}

void PickUp(PLAYER *pPlayer, spritetype *pSprite)
{
	const char *msg = nullptr;
    int nType = pSprite->type;
    char pickedUp = 0;
    int customMsg = -1;
    #ifdef NOONE_EXTENSIONS
        if (gModernMap) { // allow custom INI message instead "Picked up"
            XSPRITE* pXSprite = (pSprite->extra >= 0) ? &xsprite[pSprite->extra] : NULL;
            if (pXSprite != NULL && pXSprite->txID != 3 && pXSprite->lockMsg > 0)
                customMsg = pXSprite->lockMsg;
        }
    #endif

    if (nType >= kItemBase && nType <= kItemMax) {
        pickedUp = PickupItem(pPlayer, pSprite);
        if (pickedUp && customMsg == -1) msg = GStrings(FStringf("TXTB_ITEM%02d", int(nType - kItemBase +1)));
    
    } else if (nType >= kItemAmmoBase && nType < kItemAmmoMax) {
        pickedUp = PickupAmmo(pPlayer, pSprite);
        if (pickedUp && customMsg == -1) msg = GStrings(FStringf("TXTB_AMMO%02d", int(nType - kItemAmmoBase +1)));
    
    } else if (nType >= kItemWeaponBase && nType < kItemWeaponMax) {
        pickedUp = PickupWeapon(pPlayer, pSprite);
        if (pickedUp && customMsg == -1) msg = GStrings(FStringf("TXTB_WPN%02d", int(nType - kItemWeaponBase +1)));
    }

    if (!pickedUp) return;
    else if (pSprite->extra > 0) {
        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSprite->Pickup)
            trTriggerSprite(pSprite->index, pXSprite, kCmdSpritePickup);
    }
        
    if (!actCheckRespawn(pSprite)) 
        actPostSprite(pSprite->index, kStatFree);

    pPlayer->pickupEffect = 30;
    if (pPlayer == gMe) {
        if (customMsg > 0) trTextOver(customMsg - 1);
        else if (msg) viewSetMessage(msg, 0, MESSAGE_PRIORITY_PICKUP);
    }
}

void CheckPickUp(PLAYER *pPlayer)
{
    spritetype *pSprite = pPlayer->pSprite;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector = pSprite->sectnum;
    int nNextSprite;
    for (int nSprite = headspritestat[kStatItem]; nSprite >= 0; nSprite = nNextSprite) {
        spritetype *pItem = &sprite[nSprite];
        nNextSprite = nextspritestat[nSprite];
        if (pItem->flags&32)
            continue;
        int dx = klabs(x-pItem->x)>>4;
        if (dx > 48)
            continue;
        int dy = klabs(y-pItem->y)>>4;
        if (dy > 48)
            continue;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int vb = 0;
        if (pItem->z < top)
            vb = (top-pItem->z)>>8;
        else if (pItem->z > bottom)
            vb = (pItem->z-bottom)>>8;
        if (vb > 32)
            continue;
        if (approxDist(dx,dy) > 48)
            continue;
        GetSpriteExtents(pItem, &top, &bottom);
        if (cansee(x, y, z, nSector, pItem->x, pItem->y, pItem->z, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, top, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, bottom, pItem->sectnum))
            PickUp(pPlayer, pItem);
    }
}

int ActionScan(PLAYER *pPlayer, int *a2, int *a3)
{
    *a2 = 0;
    *a3 = 0;
    spritetype *pSprite = pPlayer->pSprite;
    int x = Cos(pSprite->ang)>>16;
    int y = Sin(pSprite->ang)>>16;
    int z = pPlayer->slope;
    int hit = HitScan(pSprite, pPlayer->zView, x, y, z, 0x10000040, 128);
    int hitDist = approxDist(pSprite->x-gHitInfo.hitx, pSprite->y-gHitInfo.hity)>>4;
    if (hitDist < 64)
    {
        switch (hit)
        {
        case 3:
            *a2 = gHitInfo.hitsprite;
            *a3 = sprite[*a2].extra;
            if (*a3 > 0 && sprite[*a2].statnum == kStatThing)
            {
                spritetype *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                if (pSprite->type == kThingDroppedLifeLeech)
                {
                    if (gGameOptions.nGameType > 1 && sub_3A158(pPlayer, pSprite))
                        return -1;
                    pXSprite->data4 = pPlayer->nPlayer;
                    pXSprite->isTriggered = 0;
                }
            }
            if (*a3 > 0 && xsprite[*a3].Push)
                return 3;
            if (sprite[*a2].statnum == kStatDude)
            {
                spritetype *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                int nMass = getDudeInfo(pSprite->type)->mass;
                if (nMass)
                {
                    int t2 = divscale(0xccccc, nMass, 8);
                    xvel[*a2] += mulscale16(x, t2);
                    yvel[*a2] += mulscale16(y, t2);
                    zvel[*a2] += mulscale16(z, t2);
                }
                if (pXSprite->Push && !pXSprite->state && !pXSprite->isTriggered)
                    trTriggerSprite(*a2, pXSprite, kCmdSpritePush);
            }
            break;
        case 0:
        case 4:
            *a2 = gHitInfo.hitwall;
            *a3 = wall[*a2].extra;
            if (*a3 > 0 && xwall[*a3].triggerPush)
                return 0;
            if (wall[*a2].nextsector >= 0)
            {
                *a2 = wall[*a2].nextsector;
                *a3 = sector[*a2].extra;
                if (*a3 > 0 && xsector[*a3].Wallpush)
                    return 6;
            }
            break;
        case 1:
        case 2:
            *a2 = gHitInfo.hitsect;
            *a3 = sector[*a2].extra;
            if (*a3 > 0 && xsector[*a3].Push)
                return 6;
            break;
        }
    }
    *a2 = pSprite->sectnum;
    *a3 = sector[*a2].extra;
    if (*a3 > 0 && xsector[*a3].Push)
        return 6;
    return -1;
}

void ProcessInput(PLAYER *pPlayer)
{
    enum
    {
        Item_MedKit = 0,
        Item_CrystalBall = 1,
        Item_BeastVision = 2,
        Item_JumpBoots = 3
    };

    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nSprite = pPlayer->nSprite;
    POSTURE *pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
    InputPacket *pInput = &pPlayer->input;

    if (pPlayer == gMe && numplayers == 1)
    {
        gViewAngleAdjust = 0.f;
        gViewLookRecenter = false;
        gViewLookAdjust = 0.f;
    }

    pPlayer->isRunning = !!(pInput->actions & SB_RUN);
    if ((pInput->actions & SB_BUTTON_MASK) || pInput->fvel || pInput->svel || pInput->q16avel)
        pPlayer->restTime = 0;
    else if (pPlayer->restTime >= 0)
        pPlayer->restTime += 4;
    WeaponProcess(pPlayer);
    if (pXSprite->health == 0)
    {
        char bSeqStat = playerSeqPlaying(pPlayer, 16);
        if (pPlayer->fraggerId != -1)
        {
            pPlayer->angold = pSprite->ang = getangle(sprite[pPlayer->fraggerId].x - pSprite->x, sprite[pPlayer->fraggerId].y - pSprite->y);
            pPlayer->q16ang = fix16_from_int(pSprite->ang);
        }
        pPlayer->deathTime += 4;
        if (!bSeqStat)
        {
            if (bVanilla)
                pPlayer->q16horiz = fix16_from_int(mulscale16(0x8000-(Cos(ClipHigh(pPlayer->deathTime*8, 1024))>>15), 120));
            else
                pPlayer->q16horiz = mulscale16(0x8000-(Cos(ClipHigh(pPlayer->deathTime*8, 1024))>>15), fix16_from_int(120));
        }
        if (pPlayer->curWeapon)
            pInput->setNewWeapon(pPlayer->curWeapon);
        if (pInput->actions & SB_OPEN)
        {
            if (bSeqStat)
            {
                if (pPlayer->deathTime > 360)
                    seqSpawn(pPlayer->pDudeInfo->seqStartID+14, 3, pPlayer->pSprite->extra, nPlayerSurviveClient);
            }
            else if (seqGetStatus(3, pPlayer->pSprite->extra) < 0)
            {
                if (pPlayer->pSprite)
                    pPlayer->pSprite->type = kThingBloodChunks;
                actPostSprite(pPlayer->nSprite, kStatThing);
                seqSpawn(pPlayer->pDudeInfo->seqStartID+15, 3, pPlayer->pSprite->extra, -1);
                playerReset(pPlayer);
                if (gGameOptions.nGameType == 0 && numplayers == 1)
                {
                    gameRestart = 1;
                }
                else
                    playerStart(pPlayer->nPlayer);
            }
            pInput->actions &= ~SB_OPEN;
        }
        return;
    }
    if (pPlayer->posture == 1)
    {
        int x = Cos(pSprite->ang);
        int y = Sin(pSprite->ang);
        if (pInput->fvel)
        {
            int forward = pInput->fvel;
            if (forward > 0)
                forward = mulscale8(pPosture->frontAccel, forward);
            else
                forward = mulscale8(pPosture->backAccel, forward);
            xvel[nSprite] += mulscale30(forward, x);
            yvel[nSprite] += mulscale30(forward, y);
        }
        if (pInput->svel)
        {
            int strafe = pInput->svel;
            strafe = mulscale8(pPosture->sideAccel, strafe);
            xvel[nSprite] += mulscale30(strafe, y);
            yvel[nSprite] -= mulscale30(strafe, x);
        }
    }
    else if (pXSprite->height < 256)
    {
        int speed = 0x10000;
        if (pXSprite->height > 0)
            speed -= divscale16(pXSprite->height, 256);
        int x = Cos(pSprite->ang);
        int y = Sin(pSprite->ang);
        if (pInput->fvel)
        {
            int forward = pInput->fvel;
            if (forward > 0)
                forward = mulscale8(pPosture->frontAccel, forward);
            else
                forward = mulscale8(pPosture->backAccel, forward);
            if (pXSprite->height)
                forward = mulscale16(forward, speed);
            xvel[nSprite] += mulscale30(forward, x);
            yvel[nSprite] += mulscale30(forward, y);
        }
        if (pInput->svel)
        {
            int strafe = pInput->svel;
            strafe = mulscale8(pPosture->sideAccel, strafe);
            if (pXSprite->height)
                strafe = mulscale16(strafe, speed);
            xvel[nSprite] += mulscale30(strafe, y);
            yvel[nSprite] -= mulscale30(strafe, x);
        }
    }
    if (pInput->q16avel)
        pPlayer->q16ang = (pPlayer->q16ang+pInput->q16avel)&0x7ffffff;
    if (pInput->actions & SB_TURNAROUND)
    {
        if (!pPlayer->spin)
            pPlayer->spin = -1024;
        pInput->actions &= ~SB_TURNAROUND;
    }
    if (pPlayer->spin < 0)
    {
        int speed;
        if (pPlayer->posture == 1)
            speed = 64;
        else
            speed = 128;
        pPlayer->spin = min(pPlayer->spin+speed, 0);
        pPlayer->q16ang += fix16_from_int(speed);
        if (pPlayer == gMe && numplayers == 1)
            gViewAngleAdjust += float(speed);
    }
    if (pPlayer == gMe && numplayers == 1)
        gViewAngleAdjust += float(pSprite->ang - pPlayer->angold);
    pPlayer->q16ang = (pPlayer->q16ang+fix16_from_int(pSprite->ang-pPlayer->angold))&0x7ffffff;
    pPlayer->angold = pSprite->ang = fix16_to_int(pPlayer->q16ang);
    if (!(pInput->actions & SB_JUMP))
        pPlayer->cantJump = 0;

    switch (pPlayer->posture) {
    case 1:
        if (pInput->actions & SB_JUMP)
            zvel[nSprite] -= pPosture->normalJumpZ;//0x5b05;
        if (pInput->actions & SB_CROUCH)
            zvel[nSprite] += pPosture->normalJumpZ;//0x5b05;
        break;
    case 2:
        if (!(pInput->actions & SB_CROUCH))
            pPlayer->posture = 0;
        break;
    default:
        if (!pPlayer->cantJump && (pInput->actions & SB_JUMP) && pXSprite->height == 0) {
            #ifdef NOONE_EXTENSIONS
            if ((packItemActive(pPlayer, 4) && pPosture->pwupJumpZ != 0) || pPosture->normalJumpZ != 0)
            #endif
                sfxPlay3DSound(pSprite, 700, 0, 0);

            if (packItemActive(pPlayer, 4)) zvel[nSprite] = pPosture->pwupJumpZ; //-0x175555;
            else zvel[nSprite] = pPosture->normalJumpZ; //-0xbaaaa;
            pPlayer->cantJump = 1;
        }

        if (pInput->actions & SB_CROUCH)
            pPlayer->posture = 2;
        break;
    }
    if (pInput->actions & SB_OPEN)
    {
        int a2, a3;
        int hit = ActionScan(pPlayer, &a2, &a3);
        switch (hit)
        {
        case 6:
            if (a3 > 0 && a3 <= 2048)
            {
                XSECTOR *pXSector = &xsector[a3];
                int key = pXSector->Key;
                if (pXSector->locked && pPlayer == gMe)
                {
                    viewSetMessage(GStrings("TXTB_LOCKED"));
                    sndStartSample(3062, 255, 2, 0);
                }
                if (!key || pPlayer->hasKey[key])
                    trTriggerSector(a2, pXSector, kCmdSpritePush);
                else if (pPlayer == gMe)
                {
                    viewSetMessage(GStrings("TXTB_KEY"));
                    sndStartSample(3063, 255, 2, 0);
                }
            }
            break;
        case 0:
        {
            XWALL *pXWall = &xwall[a3];
            int key = pXWall->key;
            if (pXWall->locked && pPlayer == gMe)
            {
                viewSetMessage(GStrings("TXTB_LOCKED"));
                sndStartSample(3062, 255, 2, 0);
            }
            if (!key || pPlayer->hasKey[key])
                trTriggerWall(a2, pXWall, kCmdWallPush);
            else if (pPlayer == gMe)
            {
                viewSetMessage(GStrings("TXTB_KEY"));
                sndStartSample(3063, 255, 2, 0);
            }
            break;
        }
        case 3:
        {
            XSPRITE *pXSprite = &xsprite[a3];
            int key = pXSprite->key;
            if (pXSprite->locked && pPlayer == gMe && pXSprite->lockMsg)
                trTextOver(pXSprite->lockMsg);
            if (!key || pPlayer->hasKey[key])
                trTriggerSprite(a2, pXSprite, kCmdSpritePush);
            else if (pPlayer == gMe)
            {
                viewSetMessage(GStrings("TXTB_KEY"));
                sndStartSample(3063, 255, 2, 0);
            }
            break;
        }
        }
        if (pPlayer->handTime > 0)
            pPlayer->handTime = ClipLow(pPlayer->handTime-4*(6-gGameOptions.nDifficulty), 0);
        if (pPlayer->handTime <= 0 && pPlayer->hand)
        {
            spritetype *pSprite2 = actSpawnDude(pPlayer->pSprite, kDudeHand, pPlayer->pSprite->clipdist<<1, 0);
            pSprite2->ang = (pPlayer->pSprite->ang+1024)&2047;
            int nSprite = pPlayer->pSprite->index;
            int x = Cos(pPlayer->pSprite->ang)>>16;
            int y = Sin(pPlayer->pSprite->ang)>>16;
            xvel[pSprite2->index] = xvel[nSprite] + mulscale14(0x155555, x);
            yvel[pSprite2->index] = yvel[nSprite] + mulscale14(0x155555, y);
            zvel[pSprite2->index] = zvel[nSprite];
            pPlayer->hand = 0;
        }
        pInput->actions &= ~SB_OPEN;
    }
    if (bVanilla)
    {
        if ((pInput->actions & SB_CENTERVIEW) && !pInput->actions & (SB_LOOK_UP|SB_LOOK_DOWN))
        {
            if (pPlayer->q16look < 0)
                pPlayer->q16look = fix16_min(pPlayer->q16look+fix16_from_int(4), fix16_from_int(0));
            if (pPlayer->q16look > 0)
                pPlayer->q16look = fix16_max(pPlayer->q16look-fix16_from_int(4), fix16_from_int(0));
            if (!pPlayer->q16look)
                pInput->actions &= ~SB_CENTERVIEW;
        }
        else
        {
            if (pInput->actions & (SB_LOOK_UP|SB_AIM_UP))
                pPlayer->q16look = fix16_min(pPlayer->q16look+fix16_from_int(4), fix16_from_int(60));
            if (pInput->actions & (SB_LOOK_DOWN|SB_AIM_DOWN))
                pPlayer->q16look = fix16_max(pPlayer->q16look-fix16_from_int(4), fix16_from_int(-60));
        }
        pPlayer->q16look = fix16_clamp(pPlayer->q16look+pInput->q16horz, fix16_from_int(-60), fix16_from_int(60));
        if (pPlayer->q16look > 0)
            pPlayer->q16horiz = fix16_from_int(mulscale30(120, Sin(fix16_to_int(pPlayer->q16look)<<3)));
        else if (pPlayer->q16look < 0)
            pPlayer->q16horiz = fix16_from_int(mulscale30(180, Sin(fix16_to_int(pPlayer->q16look)<<3)));
        else
            pPlayer->q16horiz = 0;
    }
    else
    {
        int upAngle = 289;
        int downAngle = -347;
        double lookStepUp = 4.0*upAngle/60.0;
        double lookStepDown = -4.0*downAngle/60.0;
        if ((pInput->actions & SB_CENTERVIEW) && !pInput->actions & (SB_LOOK_UP | SB_LOOK_DOWN))
        {
            if (pPlayer->q16look < 0)
                pPlayer->q16look = fix16_min(pPlayer->q16look+fix16_from_dbl(lookStepDown), fix16_from_int(0));
            if (pPlayer->q16look > 0)
                pPlayer->q16look = fix16_max(pPlayer->q16look-fix16_from_dbl(lookStepUp), fix16_from_int(0));
            if (!pPlayer->q16look)
                pInput->actions &= ~SB_CENTERVIEW;
        }
        else
        {
            if (pInput->actions & (SB_LOOK_UP | SB_AIM_UP))
                pPlayer->q16look = fix16_min(pPlayer->q16look+fix16_from_dbl(lookStepUp), fix16_from_int(upAngle));
            if (pInput->actions & (SB_LOOK_DOWN | SB_AIM_DOWN))
                pPlayer->q16look = fix16_max(pPlayer->q16look-fix16_from_dbl(lookStepDown), fix16_from_int(downAngle));
        }
        if (pPlayer == gMe && numplayers == 1)
        {
            if (pInput->actions & (SB_LOOK_UP | SB_AIM_UP))
            {
                gViewLookAdjust += float(lookStepUp);
            }
            if (pInput->actions & (SB_LOOK_DOWN | SB_AIM_DOWN))
            {
                gViewLookAdjust -= float(lookStepDown);
            }
            gViewLookRecenter = ((pInput->actions & SB_CENTERVIEW) && !pInput->actions & (SB_LOOK_UP | SB_LOOK_DOWN));
        }
        pPlayer->q16look = fix16_clamp(pPlayer->q16look+(pInput->q16horz<<3), fix16_from_int(downAngle), fix16_from_int(upAngle));
        pPlayer->q16horiz = fix16_from_float(100.f*tanf(fix16_to_float(pPlayer->q16look)*fPI/1024.f));
    }
    int nSector = pSprite->sectnum;
    int florhit = gSpriteHit[pSprite->extra].florhit & 0xc000;
    char va;
    if (pXSprite->height < 16 && (florhit == 0x4000 || florhit == 0))
        va = 1;
    else
        va = 0;
    if (va && (sector[nSector].floorstat&2))
    {
        int z1 = getflorzofslope(nSector, pSprite->x, pSprite->y);
        int x2 = pSprite->x+mulscale30(64, Cos(pSprite->ang));
        int y2 = pSprite->y+mulscale30(64, Sin(pSprite->ang));
        short nSector2 = nSector;
        updatesector(x2, y2, &nSector2);
        if (nSector2 == nSector)
        {
            int z2 = getflorzofslope(nSector2, x2, y2);
            pPlayer->q16slopehoriz = interpolate(pPlayer->q16slopehoriz, fix16_from_int(z1-z2)>>3, 0x4000);
        }
    }
    else
    {
        pPlayer->q16slopehoriz = interpolate(pPlayer->q16slopehoriz, fix16_from_int(0), 0x4000);
        if (klabs(pPlayer->q16slopehoriz) < 4)
            pPlayer->q16slopehoriz = 0;
    }
    pPlayer->slope = (-fix16_to_int(pPlayer->q16horiz))<<7;
    if (pInput->actions & SB_INVPREV)
    {
        pInput->actions&= ~SB_INVPREV;
        packPrevItem(pPlayer);
    }
    if (pInput->actions & SB_INVNEXT)
    {
        pInput->actions &= ~SB_INVNEXT;
        packNextItem(pPlayer);
    }
    if (pInput->actions & SB_INVUSE)
    {
        pInput->actions &= ~SB_INVUSE;
        if (pPlayer->packSlots[pPlayer->packItemId].curAmount > 0)
            packUseItem(pPlayer, pPlayer->packItemId);
    }
    if (pInput->isItemUsed(Item_BeastVision))
    {
        pInput->clearItemUsed(Item_BeastVision);
        if (pPlayer->packSlots[3].curAmount > 0)
            packUseItem(pPlayer, 3);
    }
    if (pInput->isItemUsed(Item_CrystalBall))
    {
        pInput->clearItemUsed(Item_CrystalBall);
        if (pPlayer->packSlots[2].curAmount > 0)
            packUseItem(pPlayer, 2);
    }
    if (pInput->isItemUsed(Item_JumpBoots))
    {
        pInput->clearItemUsed(Item_JumpBoots);
        if (pPlayer->packSlots[4].curAmount > 0)
            packUseItem(pPlayer, 4);
    }
    if (pInput->isItemUsed(Item_MedKit))
    {
        pInput->clearItemUsed(Item_MedKit);
        if (pPlayer->packSlots[0].curAmount > 0)
            packUseItem(pPlayer, 0);
    }
    if (pInput->actions & SB_HOLSTER)
    {
        pInput->actions &= ~SB_HOLSTER;
        if (pPlayer->curWeapon)
        {
            WeaponLower(pPlayer);
            viewSetMessage("Holstering weapon");
        }
    }
    CheckPickUp(pPlayer);
}

void playerProcess(PLAYER *pPlayer)
{
    spritetype *pSprite = pPlayer->pSprite;
    int nSprite = pPlayer->nSprite;
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
    powerupProcess(pPlayer);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int dzb = (bottom-pSprite->z)/4;
    int dzt = (pSprite->z-top)/4;
    int dw = pSprite->clipdist<<2;
    if (!gNoClip)
    {
        short nSector = pSprite->sectnum;
        if (pushmove_old(&pSprite->x, &pSprite->y, &pSprite->z, &nSector, dw, dzt, dzb, CLIPMASK0) == -1)
            actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_0, 500<<4);
        if (pSprite->sectnum != nSector)
        {
            if (nSector == -1)
            {
                nSector = pSprite->sectnum;
                actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_0, 500<<4);
            }
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }
    }
    ProcessInput(pPlayer);
    int nSpeed = approxDist(xvel[nSprite], yvel[nSprite]);
    pPlayer->zViewVel = interpolate(pPlayer->zViewVel, zvel[nSprite], 0x7000);
    int dz = pPlayer->pSprite->z-pPosture->eyeAboveZ-pPlayer->zView;
    if (dz > 0)
        pPlayer->zViewVel += mulscale16(dz<<8, 0xa000);
    else
        pPlayer->zViewVel += mulscale16(dz<<8, 0x1800);
    pPlayer->zView += pPlayer->zViewVel>>8;
    pPlayer->zWeaponVel = interpolate(pPlayer->zWeaponVel, zvel[nSprite], 0x5000);
    dz = pPlayer->pSprite->z-pPosture->weaponAboveZ-pPlayer->zWeapon;
    if (dz > 0)
        pPlayer->zWeaponVel += mulscale16(dz<<8, 0x8000);
    else
        pPlayer->zWeaponVel += mulscale16(dz<<8, 0xc00);
    pPlayer->zWeapon += pPlayer->zWeaponVel>>8;
    pPlayer->bobPhase = ClipLow(pPlayer->bobPhase-4, 0);
    nSpeed >>= 16;
    if (pPlayer->posture == 1)
    {
        pPlayer->bobAmp = (pPlayer->bobAmp+17)&2047;
        pPlayer->swayAmp = (pPlayer->swayAmp+17)&2047;
        pPlayer->bobHeight = mulscale30(pPosture->bobV*10, Sin(pPlayer->bobAmp*2));
        pPlayer->bobWidth = mulscale30(pPosture->bobH*pPlayer->bobPhase, Sin(pPlayer->bobAmp-256));
        pPlayer->swayHeight = mulscale30(pPosture->swayV*pPlayer->bobPhase, Sin(pPlayer->swayAmp*2));
        pPlayer->swayWidth = mulscale30(pPosture->swayH*pPlayer->bobPhase, Sin(pPlayer->swayAmp-0x155));
    }
    else
    {
        if (pXSprite->height < 256)
        {
            pPlayer->bobAmp = (pPlayer->bobAmp+pPosture->pace[pPlayer->isRunning]*4) & 2047;
            pPlayer->swayAmp = (pPlayer->swayAmp+(pPosture->pace[pPlayer->isRunning]*4)/2) & 2047;
            if (pPlayer->isRunning)
            {
                if (pPlayer->bobPhase < 60)
                    pPlayer->bobPhase = ClipHigh(pPlayer->bobPhase+nSpeed, 60);
            }
            else
            {
                if (pPlayer->bobPhase < 30)
                    pPlayer->bobPhase = ClipHigh(pPlayer->bobPhase+nSpeed, 30);
            }
        }
        pPlayer->bobHeight = mulscale30(pPosture->bobV*pPlayer->bobPhase, Sin(pPlayer->bobAmp*2));
        pPlayer->bobWidth = mulscale30(pPosture->bobH*pPlayer->bobPhase, Sin(pPlayer->bobAmp-256));
        pPlayer->swayHeight = mulscale30(pPosture->swayV*pPlayer->bobPhase, Sin(pPlayer->swayAmp*2));
        pPlayer->swayWidth = mulscale30(pPosture->swayH*pPlayer->bobPhase, Sin(pPlayer->swayAmp-0x155));
    }
    pPlayer->flickerEffect = 0;
    pPlayer->quakeEffect = ClipLow(pPlayer->quakeEffect-4, 0);
    pPlayer->tiltEffect = ClipLow(pPlayer->tiltEffect-4, 0);
    pPlayer->visibility = ClipLow(pPlayer->visibility-4, 0);
    pPlayer->painEffect = ClipLow(pPlayer->painEffect-4, 0);
    pPlayer->blindEffect = ClipLow(pPlayer->blindEffect-4, 0);
    pPlayer->pickupEffect = ClipLow(pPlayer->pickupEffect-4, 0);
    if (pPlayer == gMe && gMe->pXSprite->health == 0)
        pPlayer->hand = 0;
    if (!pXSprite->health)
        return;
    pPlayer->isUnderwater = 0;
    if (pPlayer->posture == 1)
    {
        pPlayer->isUnderwater = 1;
        int nSector = pSprite->sectnum;
        int nLink = gLowerLink[nSector];
        if (nLink > 0 && (sprite[nLink].type == kMarkerLowGoo || sprite[nLink].type == kMarkerLowWater))
        {
            if (getceilzofslope(nSector, pSprite->x, pSprite->y) > pPlayer->zView)
                pPlayer->isUnderwater = 0;
        }
    }
    if (!pPlayer->isUnderwater)
    {
        pPlayer->underwaterTime = 1200;
        pPlayer->chokeEffect = 0;
        if (packItemActive(pPlayer, 1))
            packUseItem(pPlayer, 1);
    }
    int nType = kDudePlayer1-kDudeBase;
    switch (pPlayer->posture)
    {
    case 1:
        seqSpawn(dudeInfo[nType].seqStartID+9, 3, nXSprite, -1);
        break;
    case 2:
        seqSpawn(dudeInfo[nType].seqStartID+10, 3, nXSprite, -1);
        break;
    default:
        if (!nSpeed)
            seqSpawn(dudeInfo[nType].seqStartID, 3, nXSprite, -1);
        else
            seqSpawn(dudeInfo[nType].seqStartID+8, 3, nXSprite, -1);
        break;
    }
}

spritetype *playerFireMissile(PLAYER *pPlayer, int a2, int a3, int a4, int a5, int a6)
{
    return actFireMissile(pPlayer->pSprite, a2, pPlayer->zWeapon-pPlayer->pSprite->z, a3, a4, a5, a6);
}

spritetype * playerFireThing(PLAYER *pPlayer, int a2, int a3, int thingType, int a5)
{
    dassert(thingType >= kThingBase && thingType < kThingMax);
    return actFireThing(pPlayer->pSprite, a2, pPlayer->zWeapon-pPlayer->pSprite->z, pPlayer->slope+a3, thingType, a5);
}

void playerFrag(PLAYER *pKiller, PLAYER *pVictim)
{
    dassert(pKiller != NULL);
    dassert(pVictim != NULL);
    
    char buffer[128] = "";
    int nKiller = pKiller->pSprite->type-kDudePlayer1;
    dassert(nKiller >= 0 && nKiller < kMaxPlayers);
    int nVictim = pVictim->pSprite->type-kDudePlayer1;
    dassert(nVictim >= 0 && nVictim < kMaxPlayers);
    if (nKiller == nVictim)
    {
        pVictim->fraggerId = -1;
        if (VanillaMode() || gGameOptions.nGameType != 1)
        {
            pVictim->fragCount--;
            pVictim->fragInfo[nVictim]--;
        }
        if (gGameOptions.nGameType == 3)
            dword_21EFB0[pVictim->teamId]--;
        int nMessage = Random(5);
        int nSound = gSuicide[nMessage].at4;
        if (pVictim == gMe && gMe->handTime <= 0)
        {
			strcpy(buffer, GStrings("TXTB_KILLSELF"));
            if (gGameOptions.nGameType > 0 && nSound >= 0)
                sndStartSample(nSound, 255, 2, 0);
        }
        else
        {
            sprintf(buffer, gSuicide[nMessage].at0, gProfile[nVictim].name);
        }
    }
    else
    {
        if (VanillaMode() || gGameOptions.nGameType != 1)
        {
            pKiller->fragCount++;
            pKiller->fragInfo[nKiller]++;
        }
        if (gGameOptions.nGameType == 3)
        {
            if (pKiller->teamId == pVictim->teamId)
                dword_21EFB0[pKiller->teamId]--;
            else
            {
                dword_21EFB0[pKiller->teamId]++;
                dword_21EFD0[pKiller->teamId]+=120;
            }
        }
        int nMessage = Random(25);
        int nSound = gVictory[nMessage].at4;
        const char* pzMessage = gVictory[nMessage].at0;
        sprintf(buffer, pzMessage, gProfile[nKiller].name, gProfile[nVictim].name);
        if (gGameOptions.nGameType > 0 && nSound >= 0 && pKiller == gMe)
            sndStartSample(nSound, 255, 2, 0);
    }
    viewSetMessage(buffer);
}

void FragPlayer(PLAYER *pPlayer, int nSprite)
{
    spritetype *pSprite = NULL;
    if (nSprite >= 0)
        pSprite = &sprite[nSprite];
    if (pSprite && IsPlayerSprite(pSprite))
    {
        PLAYER *pKiller = &gPlayer[pSprite->type - kDudePlayer1];
        playerFrag(pKiller, pPlayer);
        int nTeam1 = pKiller->teamId&1;
        int nTeam2 = pPlayer->teamId&1;
        if (nTeam1 == 0)
        {
            if (nTeam1 != nTeam2)
                evSend(0, 0, 15, kCmdToggle);
            else
                evSend(0, 0, 16, kCmdToggle);
        }
        else
        {
            if (nTeam1 == nTeam2)
                evSend(0, 0, 16, kCmdToggle);
            else
                evSend(0, 0, 15, kCmdToggle);
        }
    }
}

int playerDamageArmor(PLAYER *pPlayer, DAMAGE_TYPE nType, int nDamage)
{
    DAMAGEINFO *pDamageInfo = &damageInfo[nType];
    int nArmorType = pDamageInfo->at0;
    if (nArmorType >= 0 && pPlayer->armor[nArmorType])
    {
#if 0
        int vbp = (nDamage*7)/8-nDamage/4;
        int v8 = pPlayer->at33e[nArmorType];
        int t = nDamage/4 + vbp * v8 / 3200;
        v8 -= t;
#endif
        int v8 = pPlayer->armor[nArmorType];
        int t = scale(v8, 0, 3200, nDamage/4, (nDamage*7)/8);
        v8 -= t;
        nDamage -= t;
        pPlayer->armor[nArmorType] = ClipLow(v8, 0);
    }
    return nDamage;
}

spritetype *sub_40A94(PLAYER *pPlayer, int a2)
{
    char buffer[80];
    spritetype *pSprite = NULL;
    switch (a2)
    {
    case kItemFlagA:
        pPlayer->hasFlag &= ~1;
        pSprite = actDropObject(pPlayer->pSprite, kItemFlagA);
        if (pSprite)
            pSprite->owner = pPlayer->used2[0];
        gBlueFlagDropped = true;
        sprintf(buffer, "%s dropped Blue Flag", gProfile[pPlayer->nPlayer].name);
        sndStartSample(8005, 255, 2, 0);
        viewSetMessage(buffer);
        break;
    case kItemFlagB:
        pPlayer->hasFlag &= ~2;
        pSprite = actDropObject(pPlayer->pSprite, kItemFlagB);
        if (pSprite)
            pSprite->owner = pPlayer->used2[1];
        gRedFlagDropped = true;
        sprintf(buffer, "%s dropped Red Flag", gProfile[pPlayer->nPlayer].name);
        sndStartSample(8004, 255, 2, 0);
        viewSetMessage(buffer);
        break;
    }
    return pSprite;
}

int playerDamageSprite(int nSource, PLAYER *pPlayer, DAMAGE_TYPE nDamageType, int nDamage)
{
    dassert(nSource < kMaxSprites);
    dassert(pPlayer != NULL);
    if (pPlayer->damageControl[nDamageType])
        return 0;
    nDamage = playerDamageArmor(pPlayer, nDamageType, nDamage);
    pPlayer->painEffect = ClipHigh(pPlayer->painEffect+(nDamage>>3), 600);

    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nXSprite = pSprite->extra;
    int nXSector = sector[pSprite->sectnum].extra;
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nDeathSeqID = -1;
    int nKneelingPlayer = -1;
    int nSprite = pSprite->index;
    char va = playerSeqPlaying(pPlayer, 16);
    if (!pXSprite->health)
    {
        if (va)
        {
            switch (nDamageType)
            {
            case DAMAGE_TYPE_5:
                nDeathSeqID = 18;
                sfxPlay3DSound(pSprite, 716, 0, 0);
                break;
            case DAMAGE_TYPE_3:
                GibSprite(pSprite, GIBTYPE_7, NULL, NULL);
                GibSprite(pSprite, GIBTYPE_15, NULL, NULL);
                pPlayer->pSprite->cstat |= 32768;
                nDeathSeqID = 17;
                break;
            default:
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                CGibPosition gibPos(pSprite->x, pSprite->y, top);
                CGibVelocity gibVel(xvel[pSprite->index]>>1, yvel[pSprite->index]>>1, -0xccccc);
                GibSprite(pSprite, GIBTYPE_27, &gibPos, &gibVel);
                GibSprite(pSprite, GIBTYPE_7, NULL, NULL);
                fxSpawnBlood(pSprite, nDamage<<4);
                fxSpawnBlood(pSprite, nDamage<<4);
                nDeathSeqID = 17;
                break;
            }
            }
        }
    }
    else
    {
        int nHealth = pXSprite->health-nDamage;
        pXSprite->health = ClipLow(nHealth, 0);
        if (pXSprite->health > 0 && pXSprite->health < 16)
        {
            nDamageType = DAMAGE_TYPE_2;
            pXSprite->health = 0;
            nHealth = -25;
        }
        if (pXSprite->health > 0)
        {
            DAMAGEINFO *pDamageInfo = &damageInfo[nDamageType];
            int nSound;
            if (nDamage >= (10<<4))
                nSound = pDamageInfo->at4[0];
            else
                nSound = pDamageInfo->at4[Random(3)];
            if (nDamageType == DAMAGE_TYPE_4 && pXSprite->medium == kMediumWater && !pPlayer->hand)
                nSound = 714;
            sfxPlay3DSound(pSprite, nSound, 0, 6);
            return nDamage;
        }
        sfxKill3DSound(pPlayer->pSprite, -1, 441);
        if (gGameOptions.nGameType == 3 && pPlayer->hasFlag) {
            if (pPlayer->hasFlag&1) sub_40A94(pPlayer, kItemFlagA);
            if (pPlayer->hasFlag&2) sub_40A94(pPlayer, kItemFlagB);
        }
        pPlayer->deathTime = 0;
        pPlayer->qavLoop = 0;
        pPlayer->curWeapon = 0;
        pPlayer->fraggerId = nSource;
        pPlayer->voodooTargets = 0;
        if (nDamageType == DAMAGE_TYPE_3 && nDamage < (9<<4))
            nDamageType = DAMAGE_TYPE_0;
        switch (nDamageType)
        {
        case DAMAGE_TYPE_3:
            sfxPlay3DSound(pSprite, 717, 0, 0);
            GibSprite(pSprite, GIBTYPE_7, NULL, NULL);
            GibSprite(pSprite, GIBTYPE_15, NULL, NULL);
            pPlayer->pSprite->cstat |= 32768;
            nDeathSeqID = 2;
            break;
        case DAMAGE_TYPE_1:
            sfxPlay3DSound(pSprite, 718, 0, 0);
            nDeathSeqID = 3;
            break;
        case DAMAGE_TYPE_4:
            nDeathSeqID = 1;
            break;
        default:
            if (nHealth < -20 && gGameOptions.nGameType >= 2 && Chance(0x4000))
            {
                DAMAGEINFO *pDamageInfo = &damageInfo[nDamageType];
                sfxPlay3DSound(pSprite, pDamageInfo->at10[0], 0, 2);
                nDeathSeqID = 16;
                nKneelingPlayer = nPlayerKneelClient;
                powerupActivate(pPlayer, kPwUpDeliriumShroom);
                pXSprite->target = nSource;
                evPost(pSprite->index, 3, 15, kCallbackFinishHim);
            }
            else
            {
                sfxPlay3DSound(pSprite, 716, 0, 0);
                nDeathSeqID = 1;
            }
            break;
        }
    }
    if (nDeathSeqID < 0)
        return nDamage;
    if (nDeathSeqID != 16)
    {
        powerupClear(pPlayer);
        if (nXSector > 0 && xsector[nXSector].Exit)
            trTriggerSector(pSprite->sectnum, &xsector[nXSector], kCmdSectorExit);
        pSprite->flags |= 7;
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (gPlayer[p].fraggerId == nSprite && gPlayer[p].deathTime > 0)
                gPlayer[p].fraggerId = -1;
        }
        FragPlayer(pPlayer, nSource);
        trTriggerSprite(nSprite, pXSprite, kCmdOff);
    }
    dassert(getSequence(pDudeInfo->seqStartID + nDeathSeqID) != NULL);
    seqSpawn(pDudeInfo->seqStartID+nDeathSeqID, 3, nXSprite, nKneelingPlayer);
    return nDamage;
}

int UseAmmo(PLAYER *pPlayer, int nAmmoType, int nDec)
{
    if (gInfiniteAmmo)
        return 9999;
    if (nAmmoType == -1)
        return 9999;
    pPlayer->ammoCount[nAmmoType] = ClipLow(pPlayer->ammoCount[nAmmoType]-nDec, 0);
    return pPlayer->ammoCount[nAmmoType];
}

void sub_41250(PLAYER *pPlayer)
{
    int v4 = pPlayer->aim.dz;
    int dz = pPlayer->zWeapon-pPlayer->pSprite->z;
    if (UseAmmo(pPlayer, 9, 0) < 8)
    {
        pPlayer->voodooTargets = 0;
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        int ang1 = (pPlayer->voodooVar1+pPlayer->vodooVar2)&2047;
        actFireVector(pPlayer->pSprite, 0, dz, Cos(ang1)>>16, Sin(ang1)>>16, v4, VECTOR_TYPE_21);
        int ang2 = (pPlayer->voodooVar1+2048-pPlayer->vodooVar2)&2047;
        actFireVector(pPlayer->pSprite, 0, dz, Cos(ang2)>>16, Sin(ang2)>>16, v4, VECTOR_TYPE_21);
    }
    pPlayer->voodooTargets = ClipLow(pPlayer->voodooTargets-1, 0);
}

void playerLandingSound(PLAYER *pPlayer)
{
    static int surfaceSound[] = {
        -1,
        600,
        601,
        602,
        603,
        604,
        605,
        605,
        605,
        600,
        605,
        605,
        605,
        604,
        603
    };
    spritetype *pSprite = pPlayer->pSprite;
    SPRITEHIT *pHit = &gSpriteHit[pSprite->extra];
    if (pHit->florhit)
    {
        if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, &sprite[pHit->florhit & 0x3fff]))
            return;
        char nSurf = tileGetSurfType(pHit->florhit);
        if (nSurf)
            sfxPlay3DSound(pSprite, surfaceSound[nSurf], -1, 0);
    }
}

void PlayerSurvive(int, int nXSprite)
{
    char buffer[80];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    actHealDude(pXSprite, 1, 2);
    if (gGameOptions.nGameType > 0 && numplayers > 1)
    {
        sfxPlay3DSound(pSprite, 3009, 0, 6);
        if (IsPlayerSprite(pSprite))
        {
            PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
            if (pPlayer == gMe)
                viewSetMessage(GStrings("TXT_LIVEAGAIM"));
            else
            {
                sprintf(buffer, "%s lives again!", gProfile[pPlayer->nPlayer].name);
                viewSetMessage(buffer);
            }
            pPlayer->newWeapon = 1;
        }
    }
}

void PlayerKneelsOver(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (gPlayer[p].pXSprite == pXSprite)
        {
            PLAYER *pPlayer = &gPlayer[p];
            playerDamageSprite(pPlayer->fraggerId, pPlayer, DAMAGE_TYPE_5, 500<<4);
            return;
        }
    }
}

class PlayerLoadSave : public LoadSave
{
public:
    virtual void Load(void);
    virtual void Save(void);
};

void PlayerLoadSave::Load(void)
{

    Read(dword_21EFB0, sizeof(dword_21EFB0));
    Read(&gNetPlayers, sizeof(gNetPlayers));
    Read(&gProfile, sizeof(gProfile));
    Read(&gPlayer, sizeof(gPlayer));
    #ifdef NOONE_EXTENSIONS
        Read(&gPlayerCtrl, sizeof(gPlayerCtrl));
    #endif
    for (int i = 0; i < gNetPlayers; i++) {
        gPlayer[i].pSprite = &sprite[gPlayer[i].nSprite];
        gPlayer[i].pXSprite = &xsprite[gPlayer[i].pSprite->extra];
        gPlayer[i].pDudeInfo = &dudeInfo[gPlayer[i].pSprite->type-kDudeBase];
        
    #ifdef NOONE_EXTENSIONS
        // load qav scene
        if (gPlayer[i].sceneQav != -1) {
            if (gPlayerCtrl[i].qavScene.qavResrc == NULL) 
                gPlayer[i].sceneQav = -1;
            else {
                QAV* pQav = playerQavSceneLoad(gPlayer[i].sceneQav);
                if (pQav) {
                    gPlayerCtrl[i].qavScene.qavResrc = pQav;
                    gPlayerCtrl[i].qavScene.qavResrc->Preload();
                } else {
                    gPlayer[i].sceneQav = -1;
                }
            }
        }
    #endif

    }
}

void PlayerLoadSave::Save(void)
{
    Write(dword_21EFB0, sizeof(dword_21EFB0));
    Write(&gNetPlayers, sizeof(gNetPlayers));
    Write(&gProfile, sizeof(gProfile));
    Write(&gPlayer, sizeof(gPlayer));
    
    #ifdef NOONE_EXTENSIONS
    Write(&gPlayerCtrl, sizeof(gPlayerCtrl));
    #endif
}

static PlayerLoadSave *myLoadSave;

void PlayerLoadSaveConstruct(void)
{
    myLoadSave = new PlayerLoadSave();
}

END_BLD_NS
