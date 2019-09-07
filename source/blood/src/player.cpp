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
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "build.h"
#include "mmulti.h"
#include "actor.h"
#include "blood.h"
#include "callback.h"
#include "config.h"
#include "controls.h"
#include "demo.h"
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
#include "sfx.h"
#include "sound.h"
#include "tile.h"
#include "triggers.h"
#include "trig.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"
#include "common_game.h"

PROFILE gProfile[kMaxPlayers];

PLAYER gPlayer[kMaxPlayers];
PLAYER *gMe, *gView;

POWERUPINFO gPowerUpInfo[kMaxPowerUps] = {
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 1, 1, 1 },
    { -1, 0, 100, 100 },
    { -1, 0, 50, 100 },
    { -1, 0, 20, 100 },
    { -1, 0, 100, 200 },
    { -1, 0, 2, 200 },
    { 783, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 }, // 13: cloak of invisibility
    { -1, 1, 3600, 432000 }, // 14: death mask (invulnerability)
    { 827, 0, 3600, 432000 }, // 15: jump boots
    { 828, 0, 3600, 432000 },
    { -1, 0, 3600, 1728000 }, // 17: guns akimbo
    { -1, 0, 3600, 432000 }, // 18: diving suit
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 }, // 21: crystal ball
    { -1, 0, 3600, 432000 },
    { 851, 0, 3600, 432000 },
    { 2428, 0, 3600, 432000 }, // 24: reflective shots
    { -1, 0, 3600, 432000 }, // 25: beast vision
    { -1, 0, 3600, 432000 }, // 26: cloak of shadow
    { -1, 0, 3600, 432000 },
    { -1, 0, 900, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 0, 3600, 432000 },
    { -1, 1, 3600, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { -1, 0, 1, 432000 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }, // dummy
    { -1, 1, 1, 1 } // kGDXItemLevelMap
};

int Handicap[] = {
    144, 208, 256, 304, 368
};

POSTURE gPosture[4][3] = {
    
    // normal human
    {
        {
            0x4000,
            0x4000,
            0x4000,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1600,
            0x1200,
            0xc00,
            0x90
        },
        {
            0x1200,
            0x1200,
            0x1200,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1400,
            0x1000,
            -0x600,
            0xb0
        },
        {
            0x2000,
            0x2000,
            0x2000,
            22,
            28,
            24,
            16,
            16,
            40,
            0x800,
            0x600,
            -0x600,
            0xb0
        },
    },

    // normal beast
    {
        {
            0x4000,
            0x4000,
            0x4000,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1600,
            0x1200,
            0xc00,
            0x90
        },
        {
            0x1200,
            0x1200,
            0x1200,
            14,
            17,
            24,
            16,
            32,
            80,
            0x1400,
            0x1000,
            -0x600,
            0xb0
        },
        {
            0x2000,
            0x2000,
            0x2000,
            22,
            28,
            24,
            16,
            16,
            40,
            0x800,
            0x600,
            -0x600,
            0xb0
        },
    },

    // shrink human
    {
        {
            10384, 
            12384, 
            12384, 
            14, 
            17, 
            24, 
            16, 
            32, 
            80, 
            5632, 
            4608, 
            3072, 
            144
        },
        {
            2108, 
            2108, 
            2108, 
            14, 
            17, 
            24,
            16, 
            32, 
            80, 
            5120, 
            4096, 
            -1536, 
            176
        },
        {
            2192, 
            3192, 
            4192,
            22, 
            28, 
            24, 
            16,
            16,
            40, 
            2048, 
            1536, 
            -1536,
            176
        },
    },

    // grown human
    {
        {
            19384, 
            15384, 
            15384, 
            14, 
            17, 
            24, 
            16, 
            32, 
            80, 
            5632, 
            4608, 
            3072, 
            144
        },
        {
            5608, 
            5608, 
            5608, 
            14, 
            17, 
            24, 
            16, 
            32, 
            80, 
            5120, 
            4096, 
            -1536, 
            176
        },
        {
            11192, 
            11192, 
            11192,
            22, 
            28, 
            24, 
            16, 
            16, 
            40, 
            2048, 
            1536, 
            -1536, 
            176
        },
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
void PlayerKeelsOver(int, int);

int nPlayerSurviveClient = seqRegisterClient(PlayerSurvive);
int nPlayerKeelClient = seqRegisterClient(PlayerKeelsOver);

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
    return pPlayer->at202[nPowerUp];
}

bool isGrown(spritetype* pSprite) {
    return (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], 29) > 0);
}

bool isShrinked(spritetype* pSprite) {
    return (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], 30) > 0);
}

bool shrinkPlayerSize(PLAYER* pPlayer, int divider) {
    pPlayer->pXSprite->scale = 256/divider;
    playerSetRace(pPlayer, kModeHumanShrink);
    return true;
}

bool growPlayerSize(PLAYER* pPlayer, int multiplier) {
    pPlayer->pXSprite->scale = 256*multiplier;
    playerSetRace(pPlayer, kModeHumanGrown);
    return true;
}

bool resetPlayerSize(PLAYER* pPlayer) {
    playerSetRace(pPlayer, kModeHuman);
    pPlayer->pXSprite->scale = 0;
    return true;
}

void deactivateSizeShrooms(PLAYER* pPlayer) {
    powerupDeactivate(pPlayer, 29);
    pPlayer->at202[29] = 0;

    powerupDeactivate(pPlayer, 30);
    pPlayer->at202[30] = 0;
}

char powerupActivate(PLAYER *pPlayer, int nPowerUp)
{
    if (powerupCheck(pPlayer, nPowerUp) > 0 && gPowerUpInfo[nPowerUp].at2)
        return 0;
    if (!pPlayer->at202[nPowerUp])
        pPlayer->at202[nPowerUp] = gPowerUpInfo[nPowerUp].at3;
    int nPack = powerupToPackItem(nPowerUp);
    if (nPack >= 0)
        pPlayer->packInfo[nPack].at0 = 1;
    switch (nPowerUp+100)
    {
    case kGDXItemMapLevel:
        gFullMap = true;
        break;
    case 130:
        if (isGrown(pPlayer->pSprite)) deactivateSizeShrooms(pPlayer);
        else shrinkPlayerSize(pPlayer, 2);
        break;
    case 129:
        if (isShrinked(pPlayer->pSprite)) deactivateSizeShrooms(pPlayer);
        else {
            growPlayerSize(pPlayer, 2);
            if (powerupCheck(&gPlayer[pPlayer->pSprite->type - kDudePlayer1], 13) > 0) {
                powerupDeactivate(pPlayer, 13);
                pPlayer->at202[13] = 0;
            }

            if (ceilIsTooLow(pPlayer->pSprite))
                actDamageSprite(pPlayer->pSprite->xvel, pPlayer->pSprite, DAMAGE_TYPE_3, 65535);
        }
        break;
    case 112:
    case 115: // jump boots
        pPlayer->ata1[0]++;
        break;
    case 124: // reflective shots
        if (pPlayer == gMe && gGameOptions.nGameType == 0)
            sfxSetReverb2(1);
        break;
    case 114: // death mask
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]++;
        break;
    case 118: // diving suit
        pPlayer->ata1[4]++;
        if (pPlayer == gMe && gGameOptions.nGameType == 0)
            sfxSetReverb(1);
        break;
    case 119:
        pPlayer->ata1[4]++;
        break;
    case 139:
        pPlayer->ata1[1]++;
        break;
    case 117: // guns akimbo
        pPlayer->atc.newWeapon = pPlayer->atbd;
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
        pPlayer->packInfo[nPack].at0 = 0;
    switch (nPowerUp+100)
    {
    case 130:
        resetPlayerSize(pPlayer);
        if (ceilIsTooLow(pPlayer->pSprite))
            actDamageSprite(pPlayer->pSprite->xvel, pPlayer->pSprite, DAMAGE_TYPE_3, 65535);
        break;
    case 129:
        resetPlayerSize(pPlayer);
        break;
    case 112:
    case 115: // jump boots
        pPlayer->ata1[0]--;
        break;
    case 114: // death mask
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]--;
        break;
    case 118: // diving suit
        pPlayer->ata1[4]--;
        if (pPlayer == gMe)
            sfxSetReverb(0);
        break;
    case 124: // reflective shots
        if (pPlayer == gMe)
            sfxSetReverb(0);
        break;
    case 119:
        pPlayer->ata1[4]--;
        break;
    case 139:
        pPlayer->ata1[1]--;
        break;
    case 117: // guns akimbo
        pPlayer->atc.newWeapon = pPlayer->atbd;
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
    pPlayer->at31d = ClipLow(pPlayer->at31d-4, 0);
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        int nPack = powerupToPackItem(i);
        if (nPack >= 0)
        {
            if (pPlayer->packInfo[nPack].at0)
            {
                pPlayer->at202[i] = ClipLow(pPlayer->at202[i]-4, 0);
                if (pPlayer->at202[i])
                    pPlayer->packInfo[nPack].at1 = (100*pPlayer->at202[i])/gPowerUpInfo[i].at3;
                else
                {
                    powerupDeactivate(pPlayer, i);
                    if (pPlayer->at321 == nPack)
                        pPlayer->at321 = 0;
                }
            }
        }
        else if (pPlayer->at202[i] > 0)
        {
            pPlayer->at202[i] = ClipLow(pPlayer->at202[i]-4, 0);
            if (!pPlayer->at202[i])
                powerupDeactivate(pPlayer, i);
        }
    }
}

void powerupClear(PLAYER *pPlayer)
{
    for (int i = kMaxPowerUps-1; i >= 0; i--)
    {
        pPlayer->at202[i] = 0;
    }
}

void powerupInit(void)
{
}

int packItemToPowerup(int nPack)
{
    int nPowerUp = -1;
    switch (nPack)
    {
    case 0:
        break;
    case 1:
        nPowerUp = 18;
        break;
    case 2:
        nPowerUp = 21;
        break;
    case 3:
        nPowerUp = 25;
        break;
    case 4:
        nPowerUp = 15;
        break;
    default:
        ThrowError("Unhandled pack item %d", nPack);
        break;
    }
    return nPowerUp;
}

int powerupToPackItem(int nPowerUp)
{
    const int jumpBoots = 15;
    const int divingSuit = 18;
    const int crystalBall = 21;
    const int beastVision = 25;

    switch (nPowerUp)
    {
    case divingSuit:
        return 1;
    case crystalBall:
        return 2;
    case beastVision:
        return 3;
    case jumpBoots:
        return 4;
    }
    return -1;
}

char packAddItem(PLAYER *pPlayer, unsigned int nPack)
{
    if (nPack <= 4)
    {
        if (pPlayer->packInfo[nPack].at1 >= 100)
            return 0;
        pPlayer->packInfo[nPack].at1 = 100;
        int nPowerUp = packItemToPowerup(nPack);
        if (nPowerUp >= 0)
            pPlayer->at202[nPowerUp] = gPowerUpInfo[nPowerUp].at3;
        if (pPlayer->at321 == -1)
            pPlayer->at321 = nPack;
        if (!pPlayer->packInfo[pPlayer->at321].at1)
            pPlayer->at321 = nPack;
    }
    else
        ThrowError("Unhandled pack item %d", nPack);
    return 1;
}

int packCheckItem(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packInfo[nPack].at1;
}

char packItemActive(PLAYER *pPlayer, int nPack)
{
    return pPlayer->packInfo[nPack].at0;
}

void packUseItem(PLAYER *pPlayer, int nPack)
{
    char v4 = 0;
    int nPowerUp = -1;
    if (pPlayer->packInfo[nPack].at1 > 0)
    {
        switch (nPack)
        {
        case 0:
        {
            XSPRITE *pXSprite = pPlayer->pXSprite;
            unsigned int health = pXSprite->health>>4;
            if (health < 100)
            {
                int heal = ClipHigh(100-health, pPlayer->packInfo[0].at1);
                actHealDude(pXSprite, heal, 100);
                pPlayer->packInfo[0].at1 -= heal;
            }
            break;
        }
        case 1:
            v4 = 1;
            nPowerUp = 18;
            break;
        case 2:
            v4 = 1;
            nPowerUp = 21;
            break;
        case 3:
            v4 = 1;
            nPowerUp = 25;
            break;
        case 4:
            v4 = 1;
            nPowerUp = 15;
            break;
        default:
            ThrowError("Unhandled pack item %d", nPack);
            return;
        }
    }
    pPlayer->at31d = 0;
    if (v4)
        powerupSetState(pPlayer, nPowerUp, pPlayer->packInfo[nPack].at0);
}

void packPrevItem(PLAYER *pPlayer)
{
    if (pPlayer->at31d > 0)
    {
        for (int nPrev = ClipLow(pPlayer->at321-1,0); nPrev >= 0; nPrev--)
        {
            if (pPlayer->packInfo[nPrev].at1)
            {
                pPlayer->at321 = nPrev;
                break;
            }
        }
    }
    pPlayer->at31d = 600;
}

void packNextItem(PLAYER *pPlayer)
{
    if (pPlayer->at31d > 0)
    {
        for (int nNext = ClipHigh(pPlayer->at321+1,5); nNext < 5; nNext++)
        {
            if (pPlayer->packInfo[nNext].at1)
            {
                pPlayer->at321 = nNext;
                break;
            }
        }
    }
    pPlayer->at31d = 600;
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
    pPlayer->at5f = nLifeMode;
    
    // By NoOne: don't forget to change clipdist for grow and shrink modes
    pPlayer->pSprite->clipdist = pDudeInfo->clipdist;
    
    for (int i = 0; i < 7; i++)
        pDudeInfo->at70[i] = mulscale8(Handicap[gProfile[pPlayer->at57].skill], pDudeInfo->startDamage[i]);
}

void playerSetGodMode(PLAYER *pPlayer, char bGodMode)
{
    if (bGodMode)
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]++;
    }
    else
    {
        for (int i = 0; i < 7; i++)
            pPlayer->ata1[i]--;
    }
    pPlayer->at31a = bGodMode;
}

void playerResetInertia(PLAYER *pPlayer)
{
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
    pPlayer->at67 = pPlayer->pSprite->z-pPosture->at24;
    pPlayer->at6f = pPlayer->pSprite->z-pPosture->at28;
    viewBackupView(pPlayer->at57);
}

void playerResetPowerUps(PLAYER* pPlayer)
{
    const int jumpBoots = 15;
    const int divingSuit = 18;
    const int crystalBall = 21;
    const int beastVision = 25;

    for (int i = 0; i < kMaxPowerUps; i++)
    {
        if (!VanillaMode()
            && (i == jumpBoots
                || i == divingSuit
                || i == crystalBall
                || i == beastVision))
            continue;

        pPlayer->at202[i] = 0;
    }
}

void playerStart(int nPlayer)
{
    PLAYER* pPlayer = &gPlayer[nPlayer];
    GINPUT* pInput = &pPlayer->atc;
    ZONE* pStartZone = NULL;

    // normal start position
    if (gGameOptions.nGameType <= 1)
        pStartZone = &gStartZone[nPlayer];
    
    // By NoOne: let's check if there is positions of teams is specified
    // if no, pick position randomly, just like it works in vanilla.
    else if (gGameOptions.nGameType == 3 && gTeamsSpawnUsed == true) {
        int maxRetries = 5;
        while (maxRetries-- > 0) {
            if (pPlayer->at2ea == 0) pStartZone = &gStartZoneTeam1[Random(3)];
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
    } else {
        pStartZone = &gStartZone[Random(8)];
    }

    spritetype *pSprite = actSpawnSprite(pStartZone->sectnum, pStartZone->x, pStartZone->y, pStartZone->z, 6, 1);
    dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    pPlayer->pSprite = pSprite;
    pPlayer->pXSprite = pXSprite;
    pPlayer->at5b = pSprite->index;
    DUDEINFO *pDudeInfo = &dudeInfo[kDudePlayer1 + nPlayer - kDudeBase];
    pPlayer->pDudeInfo = pDudeInfo;
    playerSetRace(pPlayer, kModeHuman);
    seqSpawn(pDudeInfo->seqStartID, 3, pSprite->extra, -1);
    if (pPlayer == gMe)
        SetBitString(show2dsprite, pSprite->index);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z -= bottom - pSprite->z;
    pSprite->pal = 11+(pPlayer->at2ea&3);
    pPlayer->angold = pSprite->ang = pStartZone->ang;
    pPlayer->q16ang = fix16_from_int(pSprite->ang);
    pSprite->type = kDudePlayer1+nPlayer;
    pSprite->clipdist = pDudeInfo->clipdist;
    pSprite->hitag = 15;
    pXSprite->burnTime = 0;
    pXSprite->burnSource = -1;
    pPlayer->pXSprite->health = pDudeInfo->startHealth<<4;
    pPlayer->pSprite->cstat &= (unsigned short)~32768;
    pPlayer->at63 = 0;
    pPlayer->q16horiz = 0;
    pPlayer->q16slopehoriz = 0;
    pPlayer->q16look = 0;
    pPlayer->at83 = 0;
    pPlayer->at2ee = -1;
    pPlayer->at2f2 = 1200;
    pPlayer->at2f6 = 0;
    pPlayer->at2fa = 0;
    pPlayer->at2fe = 0;
    pPlayer->at302 = 0;
    pPlayer->at306 = 0;
    pPlayer->at30a = 0;
    pPlayer->at30e = 0;
    pPlayer->at312 = 0;
    pPlayer->at316 = 0;
    pPlayer->at2f = 0;
    pPlayer->voodooTarget = -1;
    pPlayer->at34e = 0;
    pPlayer->at352 = 0;
    pPlayer->at356 = 0;
    playerResetInertia(pPlayer);
    pPlayer->at73 = 0;
    pPlayer->at1ca.dx = 0x4000;
    pPlayer->at1ca.dy = 0;
    pPlayer->at1ca.dz = 0;
    pPlayer->at1d6 = -1;
    pPlayer->at6b = pPlayer->at73;
    for (int i = 0; i < 8; i++)
        pPlayer->at88[i] = gGameOptions.nGameType >= 2;
    pPlayer->at90 = 0;
    for (int i = 0; i < 8; i++)
        pPlayer->at91[i] = -1;
    for (int i = 0; i < 7; i++)
        pPlayer->ata1[i] = 0;
    if (pPlayer->at31a)
        playerSetGodMode(pPlayer, 1);
    gInfiniteAmmo = 0;
    gFullMap = 0;
    pPlayer->at1ba = 0;
    pPlayer->at1fe = 0;
    pPlayer->atbe = 0;
    xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
    pInput->q16turn = 0;
    pInput->keyFlags.word = 0;
    pInput->forward = 0;
    pInput->strafe = 0;
    pInput->q16mlook = 0;
    pInput->buttonFlags.byte = 0;
    pInput->useFlags.byte = 0;
    pPlayer->at35a = 0;
    pPlayer->at37f = 0;
    pPlayer->at35e = 0;
    pPlayer->at362 = 0;
    pPlayer->at366 = 0;
    pPlayer->at36a = 0;
    pPlayer->at36e = 0;
    pPlayer->at372 = 0;
    pPlayer->atbf = 0;
    pPlayer->atc3 = 0;
    pPlayer->at26 = -1;
    pPlayer->at376 = 0;
    pPlayer->nWaterPal = 0;
    playerResetPowerUps(pPlayer);

    if (pPlayer == gMe)
    {
        viewInitializePrediction();
        gViewMap.x = pPlayer->pSprite->x;
        gViewMap.y = pPlayer->pSprite->y;
        gViewMap.angle = pPlayer->pSprite->ang;
    }
    if (IsUnderwaterSector(pSprite->sectnum))
    {
        pPlayer->at2f = 1;
        pPlayer->pXSprite->medium = 1;
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
        pPlayer->atcb[i] = gInfiniteAmmo;
        pPlayer->atd9[i] = 0;
    }
    pPlayer->atcb[1] = 1;
    pPlayer->atbd = 0;
    pPlayer->at2a = -1;
    pPlayer->atc.newWeapon = 1;
    for (int i = 0; i < 14; i++)
    {
        pPlayer->at111[0][i] = dword_136400[i];
        pPlayer->at111[1][i] = dword_136438[i];
    }
    for (int i = 0; i < 12; i++)
    {
        if (gInfiniteAmmo)
            pPlayer->at181[i] = gAmmoInfo[i].at0;
        else
            pPlayer->at181[i] = 0;
    }
    for (int i = 0; i < 3; i++)
        pPlayer->at33e[i] = 0;
    pPlayer->atbf = 0;
    pPlayer->atc3 = 0;
    pPlayer->at26 = -1;
    pPlayer->at1b1 = 0;
    pPlayer->at321 = -1;
    for (int i = 0; i < 5; i++)
    {
        pPlayer->packInfo[i].at0 = 0;
        pPlayer->packInfo[i].at1 = 0;
    }
}

int dword_21EFB0[8];
ClockTicks dword_21EFD0[8];

void playerInit(int nPlayer, unsigned int a2)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    if (!(a2&1))
        memset(pPlayer, 0, sizeof(PLAYER));
    pPlayer->at57 = nPlayer;
    pPlayer->at2ea = nPlayer;
    if (gGameOptions.nGameType == 3)
        pPlayer->at2ea = nPlayer&1;
    pPlayer->at2c6 = 0;
    memset(dword_21EFB0, 0, sizeof(dword_21EFB0));
    memset(dword_21EFD0, 0, sizeof(dword_21EFD0));
    memset(pPlayer->at2ca, 0, sizeof(pPlayer->at2ca));
    if (!(a2&1))
        playerReset(pPlayer);
}

char sub_3A158(PLAYER *a1, spritetype *a2)
{
    for (int nSprite = headspritestat[4]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        if (a2 && a2->index == nSprite)
            continue;
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->type == 431 && actOwnerIdToSpriteId(pSprite->owner) == a1->at5b)
            return 1;
    }
    return 0;
}

char PickupItem(PLAYER *pPlayer, spritetype *pItem)
{
    char buffer[80];
    int pickupSnd = 775;
    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nType = pItem->type - 100;
    switch (pItem->type)
    {
    //case 129:
        //dudeInfo[31].seqStartID = 13568;
        //if (!powerupActivate(pPlayer, nType))
            //return 0;
        //return 1;
    case 113:
        if (isGrown(pPlayer->pSprite)) return false;
    case 130:
    case 129:
        switch (pItem->type) {
        case 130:
            if (isShrinked(pSprite)) return false;
            break;
        case 129:
            if (isGrown(pSprite)) return false;
            break;
        }
        powerupActivate(pPlayer, nType);
        break;
    case 145:
    case 146:
        if (gGameOptions.nGameType != 3)
            return 0;
        if (pItem->extra > 0)
        {
            XSPRITE *pXItem = &xsprite[pItem->extra];
            if (pItem->type == 145)
            {
                if (pPlayer->at2ea == 1)
                {
                    if ((pPlayer->at90&1) == 0 && pXItem->state)
                    {
                        pPlayer->at90 |= 1;
                        pPlayer->at91[0] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, 0);
                        sprintf(buffer, "%s stole Blue Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8007, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }
                if (pPlayer->at2ea == 0)
                {
                    if ((pPlayer->at90&1) != 0 && !pXItem->state)
                    {
                        pPlayer->at90 &= ~1;
                        pPlayer->at91[0] = -1;
                        trTriggerSprite(pItem->index, pXItem, 1);
                        sprintf(buffer, "%s returned Blue Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8003, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                    if ((pPlayer->at90&2) != 0 && pXItem->state)
                    {
                        pPlayer->at90 &= ~2;
                        pPlayer->at91[1] = -1;
                        dword_21EFB0[pPlayer->at2ea] += 10;
                        dword_21EFD0[pPlayer->at2ea] += 240;
                        evSend(0, 0, 81, COMMAND_ID_1);
                        sprintf(buffer, "%s captured Red Flag!", gProfile[pPlayer->at57].name);
                        sndStartSample(8001, 255, 2, 0);
                        viewSetMessage(buffer);
#if 0
                        if (dword_28E3D4 == 3 && myconnectindex == connecthead)
                        {
                            sprintf(buffer, "frag A killed B\n");
                            sub_7AC28(buffer);
                        }
#endif
                    }
                }
            }
            else if (pItem->type == 146)
            {
                if (pPlayer->at2ea == 0)
                {
                    if((pPlayer->at90&2) == 0 && pXItem->state)
                    {
                        pPlayer->at90 |= 2;
                        pPlayer->at91[1] = pItem->index;
                        trTriggerSprite(pItem->index, pXItem, 0);
                        sprintf(buffer, "%s stole Red Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8006, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                }
                if (pPlayer->at2ea == 1)
                {
                    if ((pPlayer->at90&2) != 0 && !pXItem->state)
                    {
                        pPlayer->at90 &= ~2;
                        pPlayer->at91[1] = -1;
                        trTriggerSprite(pItem->index, pXItem, 1);
                        sprintf(buffer, "%s returned Red Flag", gProfile[pPlayer->at57].name);
                        sndStartSample(8002, 255, 2, 0);
                        viewSetMessage(buffer);
                    }
                    if ((pPlayer->at90&1) != 0 && pXItem->state)
                    {
                        pPlayer->at90 &= ~1;
                        pPlayer->at91[0] = -1;
                        dword_21EFB0[pPlayer->at2ea] += 10;
                        dword_21EFD0[pPlayer->at2ea] += 240;
                        evSend(0, 0, 80, COMMAND_ID_1);
                        sprintf(buffer, "%s captured Red Flag!", gProfile[pPlayer->at57].name);
                        sndStartSample(8000, 255, 2, 0);
                        viewSetMessage(buffer);
#if 0
                        if (dword_28E3D4 == 3 && myconnectindex == connecthead)
                        {
                            sprintf(buffer, "frag B killed A\n");
                            sub_7AC28(buffer);
                        }
#endif
                    }
                }
            }
        }
        return 0;
    case 147:
        if (gGameOptions.nGameType != 3)
            return 0;
        evKill(pItem->index, 3, CALLBACK_ID_17);
        pPlayer->at90 |= 1;
        pPlayer->at91[0] = pItem->index;
        break;
    case 148:
        if (gGameOptions.nGameType != 3)
            return 0;
        evKill(pItem->index, 3, CALLBACK_ID_17);
        pPlayer->at90 |= 2;
        pPlayer->at91[1] = pItem->index;
        break;
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
    {
        ARMORDATA *pArmorData = &armorData[pItem->type-140];
        char va = 0;
        if (pPlayer->at33e[1] < pArmorData->atc)
        {
            pPlayer->at33e[1] = ClipHigh(pPlayer->at33e[1]+pArmorData->at8, pArmorData->atc);
            va = 1;
        }
        if (pPlayer->at33e[0] < pArmorData->at4)
        {
            pPlayer->at33e[0] = ClipHigh(pPlayer->at33e[0]+pArmorData->at0, pArmorData->at4);
            va = 1;
        }
        if (pPlayer->at33e[2] < pArmorData->at14)
        {
            pPlayer->at33e[2] = ClipHigh(pPlayer->at33e[2]+pArmorData->at10, pArmorData->at14);
            va = 1;
        }
        if (!va)
            return 0;
        pickupSnd = 779;
        break;
    }
    case 121:
        if (gGameOptions.nGameType == 0)
            return 0;
        if (!packAddItem(pPlayer, gItemData[nType].at8))
            return 0;
        break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
        if (pPlayer->at88[pItem->type-99])
            return 0;
        pPlayer->at88[pItem->type-99] = 1;
        pickupSnd = 781;
        break;
    case 108:
    case 109:
    case 110:
    case 111: 
    {
        int addPower = gPowerUpInfo[nType].at3;
        // by NoOne: allow custom amount for item
        if (sprite[pItem->xvel].extra >= 0 && xsprite[sprite[pItem->xvel].extra].data1 > 0 && !VanillaMode() && !DemoRecordStatus())
            addPower = xsprite[sprite[pItem->xvel].extra].data1;
        if (!actHealDude(pXSprite, addPower, gPowerUpInfo[nType].at7))
            return 0;
        return 1;
    }
    case 107:
    case 115:
    case 118:
    case 125:
        if (!packAddItem(pPlayer, gItemData[nType].at8))
            return 0;
        break;
    default:
        if (!powerupActivate(pPlayer, nType))
            return 0;
        return 1;
    }
    sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, pickupSnd, pSprite->sectnum);
    return 1;
}

char PickupAmmo(PLAYER* pPlayer, spritetype* pAmmo)
{
    AMMOITEMDATA* pAmmoItemData = &gAmmoItemData[pAmmo->type - 60];
    int nAmmoType = pAmmoItemData->ata;

    if (pPlayer->at181[nAmmoType] >= gAmmoInfo[nAmmoType].at0) return 0;
    else if (pAmmo->extra < 0 || xsprite[pAmmo->extra].data1 <= 0 || VanillaMode() || DemoRecordStatus())
        pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType]+pAmmoItemData->at8, gAmmoInfo[nAmmoType].at0);
    // by NoOne: allow custom amount for item
    else
        pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType] + xsprite[pAmmo->extra].data1, gAmmoInfo[nAmmoType].at0);

    if (pAmmoItemData->atb)
        pPlayer->atcb[pAmmoItemData->atb] = 1;
    sfxPlay3DSound(pPlayer->pSprite, 782, -1, 0);
    return 1;
}

char PickupWeapon(PLAYER *pPlayer, spritetype *pWeapon)
{
    WEAPONITEMDATA *pWeaponItemData = &gWeaponItemData[pWeapon->type-40];
    int nWeaponType = pWeaponItemData->at8;
    int nAmmoType = pWeaponItemData->ata;
    if (!pPlayer->atcb[nWeaponType] || gGameOptions.nWeaponSettings == 2 || gGameOptions.nWeaponSettings == 3)
    {
        if (pWeapon->type == 50 && gGameOptions.nGameType > 1 && sub_3A158(pPlayer, NULL))
            return 0;
        pPlayer->atcb[nWeaponType] = 1;
        if (nAmmoType == -1) return 0;
        // By NoOne: allow to set custom ammo count for weapon pickups
        if (pWeapon->extra < 0 || xsprite[pWeapon->extra].data1 <= 0 || VanillaMode() || DemoRecordStatus())
            pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType] + pWeaponItemData->atc, gAmmoInfo[nAmmoType].at0);
        else
            pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType] + xsprite[pWeapon->extra].data1, gAmmoInfo[nAmmoType].at0);

        int nNewWeapon = WeaponUpgrade(pPlayer, nWeaponType);
        if (nNewWeapon != pPlayer->atbd)
        {
            pPlayer->atc3 = 0;
            pPlayer->atbe = nNewWeapon;
        }
        sfxPlay3DSound(pPlayer->pSprite, 777, -1, 0);
        return 1;
    }
    if (!actGetRespawnTime(pWeapon))
        return 0;
    if (nAmmoType == -1)
        return 0;
    if (pPlayer->at181[nAmmoType] >= gAmmoInfo[nAmmoType].at0)
        return 0;
    pPlayer->at181[nAmmoType] = ClipHigh(pPlayer->at181[nAmmoType]+pWeaponItemData->atc, gAmmoInfo[nAmmoType].at0);
    sfxPlay3DSound(pPlayer->pSprite, 777, -1, 0);
    return 1;
}

void PickUp(PLAYER *pPlayer, spritetype *pSprite)
{
    char buffer[80];
    int nType = pSprite->type;
    char pickedUp = 0;
    int customMsg = -1;
    if (nType != 40 && nType != 80) { // By NoOne: no pickup for random item generators.
        
        XSPRITE* pXSprite = (pSprite->extra >= 0) ? &xsprite[pSprite->extra] : NULL;
        if (pXSprite != NULL && pXSprite->txID != 3 && pXSprite->lockMsg > 0) // by NoOne: allow custom INI message instead "Picked up"
            customMsg = pXSprite->lockMsg;

        if (nType >= 100 && nType <= 149)
        {
            pickedUp = PickupItem(pPlayer, pSprite);
            if (pickedUp && customMsg == -1) sprintf(buffer, "Picked up %s", gItemText[nType - 100]);
        }
        else if (nType >= 60 && nType < 81)
        {
            pickedUp = PickupAmmo(pPlayer, pSprite);
            if (pickedUp && customMsg == -1) sprintf(buffer, "Picked up %s", gAmmoText[nType - 60]);
        }
        else if (nType >= 40 && nType < 51)
        {
            pickedUp = PickupWeapon(pPlayer, pSprite);
            if (pickedUp && customMsg == -1) sprintf(buffer, "Picked up %s", gWeaponText[nType - 40]);
        }
    }
    if (pickedUp)
    {
        if (pSprite->extra > 0)
        {
            XSPRITE *pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->Pickup)
                trTriggerSprite(pSprite->index, pXSprite, 32);
        }
        if (!actCheckRespawn(pSprite))
            actPostSprite(pSprite->index, kStatFree);
        pPlayer->at377 = 30;
        if (pPlayer == gMe)
            if (customMsg > 0) trTextOver(customMsg - 1);
            else viewSetMessage(buffer);
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
    for (int nSprite = headspritestat[3]; nSprite >= 0; nSprite = nNextSprite)
    {
        spritetype *pItem = &sprite[nSprite];
        nNextSprite = nextspritestat[nSprite];
        if (pItem->hitag&32)
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
    int z = pPlayer->at83;
    int hit = HitScan(pSprite, pPlayer->at67, x, y, z, 0x10000040, 128);
    int hitDist = approxDist(pSprite->x-gHitInfo.hitx, pSprite->y-gHitInfo.hity)>>4;
    if (hitDist < 64)
    {
        switch (hit)
        {
        case 3:
            *a2 = gHitInfo.hitsprite;
            *a3 = sprite[*a2].extra;
            if (*a3 > 0 && sprite[*a2].statnum == 4)
            {
                spritetype *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                if (pSprite->type == 431)
                {
                    if (gGameOptions.nGameType > 1 && sub_3A158(pPlayer, pSprite))
                        return -1;
                    pXSprite->data4 = pPlayer->at57;
                    pXSprite->isTriggered = 0;
                }
            }
            if (*a3 > 0 && xsprite[*a3].Push)
                return 3;
            if (sprite[*a2].statnum == 6)
            {
                spritetype *pSprite = &sprite[*a2];
                XSPRITE *pXSprite = &xsprite[*a3];
                int nMass = dudeInfo[pSprite->type-kDudeBase].mass;
                if (nMass)
                {
                    int t2 = divscale(0xccccc, nMass, 8);
                    xvel[*a2] += mulscale16(x, t2);
                    yvel[*a2] += mulscale16(y, t2);
                    zvel[*a2] += mulscale16(z, t2);
                }
                if (pXSprite->Push && !pXSprite->state && !pXSprite->isTriggered)
                    trTriggerSprite(*a2, pXSprite, 30);
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
    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nSprite = pPlayer->at5b;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
    GINPUT *pInput = &pPlayer->atc;
    pPlayer->at2e = pInput->syncFlags.run;
    if (pInput->buttonFlags.byte || pInput->forward || pInput->strafe || pInput->q16turn)
        pPlayer->at30a = 0;
    else if (pPlayer->at30a >= 0)
        pPlayer->at30a += 4;
    WeaponProcess(pPlayer);
    if (pXSprite->health == 0)
    {
        char bSeqStat = playerSeqPlaying(pPlayer, 16);
        if (pPlayer->at2ee != -1)
        {
            pPlayer->angold = pSprite->ang = getangle(sprite[pPlayer->at2ee].x - pSprite->x, sprite[pPlayer->at2ee].y - pSprite->y);
            pPlayer->q16ang = fix16_from_int(pSprite->ang);
        }
        pPlayer->at1fe += 4;
        if (!bSeqStat)
        {
            if (bVanilla)
                pPlayer->q16horiz = fix16_from_int(mulscale16(0x8000-(Cos(ClipHigh(pPlayer->at1fe*8, 1024))>>15), 120));
            else
                pPlayer->q16horiz = mulscale16(0x8000-(Cos(ClipHigh(pPlayer->at1fe*8, 1024))>>15), F16(120));
        }
        if (pPlayer->atbd)
            pInput->newWeapon = pPlayer->atbd;
        if (pInput->keyFlags.action)
        {
            if (bSeqStat)
            {
                if (pPlayer->at1fe > 360)
                    seqSpawn(pPlayer->pDudeInfo->seqStartID+14, 3, pPlayer->pSprite->extra, nPlayerSurviveClient);
            }
            else if (seqGetStatus(3, pPlayer->pSprite->extra) < 0)
            {
                if (pPlayer->pSprite)
                    pPlayer->pSprite->type = 426;
                actPostSprite(pPlayer->at5b, 4);
                seqSpawn(pPlayer->pDudeInfo->seqStartID+15, 3, pPlayer->pSprite->extra, -1);
                playerReset(pPlayer);
                if (gGameOptions.nGameType == 0 && numplayers == 1)
                {
                    if (gDemo.at0)
                        gDemo.Close();
                    pInput->keyFlags.restart = 1;
                }
                else
                    playerStart(pPlayer->at57);
            }
            pInput->keyFlags.action = 0;
        }
        return;
    }
    if (pPlayer->at2f == 1)
    {
        int x = Cos(pSprite->ang);
        int y = Sin(pSprite->ang);
        if (pInput->forward)
        {
            int forward = pInput->forward;
            if (forward > 0)
                forward = mulscale8(pPosture->at0, forward);
            else
                forward = mulscale8(pPosture->at8, forward);
            xvel[nSprite] += mulscale30(forward, x);
            yvel[nSprite] += mulscale30(forward, y);
        }
        if (pInput->strafe)
        {
            int strafe = pInput->strafe;
            strafe = mulscale8(pPosture->at4, strafe);
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
        if (pInput->forward)
        {
            int forward = pInput->forward;
            if (forward > 0)
                forward = mulscale8(pPosture->at0, forward);
            else
                forward = mulscale8(pPosture->at8, forward);
            if (pXSprite->height)
                forward = mulscale16(forward, speed);
            xvel[nSprite] += mulscale30(forward, x);
            yvel[nSprite] += mulscale30(forward, y);
        }
        if (pInput->strafe)
        {
            int strafe = pInput->strafe;
            strafe = mulscale8(pPosture->at4, strafe);
            if (pXSprite->height)
                strafe = mulscale16(strafe, speed);
            xvel[nSprite] += mulscale30(strafe, y);
            yvel[nSprite] -= mulscale30(strafe, x);
        }
    }
    if (pInput->q16turn)
        pPlayer->q16ang = (pPlayer->q16ang+pInput->q16turn)&0x7ffffff;
    if (pInput->keyFlags.spin180)
    {
        if (!pPlayer->at316)
            pPlayer->at316 = -1024;
        pInput->keyFlags.spin180 = 0;
    }
    if (pPlayer->at316 < 0)
    {
        int speed;
        if (pPlayer->at2f == 1)
            speed = 64;
        else
            speed = 128;
        pPlayer->at316 = ClipLow(pPlayer->at316+speed, 0);
        pPlayer->q16ang += fix16_from_int(speed);
    }
    pPlayer->q16ang = (pPlayer->q16ang+fix16_from_int(pSprite->ang-pPlayer->angold))&0x7ffffff;
    pPlayer->angold = pSprite->ang = fix16_to_int(pPlayer->q16ang);
    if (!pInput->buttonFlags.jump)
        pPlayer->at31c = 0;

    switch (pPlayer->at2f)
    {
    case 1:
        if (pInput->buttonFlags.jump)
            zvel[nSprite] -= 0x5b05;
        if (pInput->buttonFlags.crouch)
            zvel[nSprite] += 0x5b05;
        break;
    case 2:
        if (!pInput->buttonFlags.crouch)
            pPlayer->at2f = 0;
        break;
    default:
        if (!pPlayer->at31c && pInput->buttonFlags.jump && pXSprite->height == 0)
        {
            sfxPlay3DSound(pSprite, 700, 0, 0);
            if (packItemActive(pPlayer, 4))
                zvel[nSprite] = -0x175555;
            else
                zvel[nSprite] = -0xbaaaa;
            

            if (isShrinked(pPlayer->pSprite)) zvel[nSprite] -= -200000;
            else if (isGrown(pPlayer->pSprite)) zvel[nSprite] += -250000;
            
            pPlayer->at31c = 1;
        }


        if (pInput->buttonFlags.crouch)
            pPlayer->at2f = 2;
        break;
    }
    if (pInput->keyFlags.action)
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
                    viewSetMessage("It's locked");
                    sndStartSample(3062, 255, 2, 0);
                }
                if (!key || pPlayer->at88[key])
                    trTriggerSector(a2, pXSector, 30);
                else if (pPlayer == gMe)
                {
                    viewSetMessage("That requires a key.");
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
                viewSetMessage("It's locked");
                sndStartSample(3062, 255, 2, 0);
            }
            if (!key || pPlayer->at88[key])
                trTriggerWall(a2, pXWall, 50);
            else if (pPlayer == gMe)
            {
                viewSetMessage("That requires a key.");
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
            if (!key || pPlayer->at88[key])
                trTriggerSprite(a2, pXSprite, 30);
            else if (pPlayer == gMe)
            {
                viewSetMessage("That requires a key.");
                sndStartSample(3063, 255, 2, 0);
            }
            break;
        }
        }
        if (pPlayer->at372 > 0)
            pPlayer->at372 = ClipLow(pPlayer->at372-4*(6-gGameOptions.nDifficulty), 0);
        if (pPlayer->at372 <= 0 && pPlayer->at376)
        {
            spritetype *pSprite2 = actSpawnDude(pPlayer->pSprite, 212, pPlayer->pSprite->clipdist<<1, 0);
            pSprite2->ang = (pPlayer->pSprite->ang+1024)&2047;
            int nSprite = pPlayer->pSprite->index;
            int x = Cos(pPlayer->pSprite->ang)>>16;
            int y = Sin(pPlayer->pSprite->ang)>>16;
            xvel[pSprite2->index] = xvel[nSprite] + mulscale14(0x155555, x);
            yvel[pSprite2->index] = yvel[nSprite] + mulscale14(0x155555, y);
            zvel[pSprite2->index] = zvel[nSprite];
            pPlayer->at376 = 0;
        }
        pInput->keyFlags.action = 0;
    }
    if (bVanilla)
    {
        if (pInput->keyFlags.lookCenter && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
        {
            if (pPlayer->q16look < 0)
                pPlayer->q16look = fix16_min(pPlayer->q16look+F16(4), F16(0));
            if (pPlayer->q16look > 0)
                pPlayer->q16look = fix16_max(pPlayer->q16look-F16(4), F16(0));
            if (!pPlayer->q16look)
                pInput->keyFlags.lookCenter = 0;
        }
        else
        {
            if (pInput->buttonFlags.lookUp)
                pPlayer->q16look = fix16_min(pPlayer->q16look+F16(4), F16(60));
            if (pInput->buttonFlags.lookDown)
                pPlayer->q16look = fix16_max(pPlayer->q16look-F16(4), F16(-60));
        }
        pPlayer->q16look = fix16_clamp(pPlayer->q16look+pInput->q16mlook, F16(-60), F16(60));
        if (pPlayer->q16look > 0)
            pPlayer->q16horiz = fix16_from_int(mulscale30(120, Sin(fix16_to_int(pPlayer->q16look)<<3)));
        else if (pPlayer->q16look < 0)
            pPlayer->q16horiz = fix16_from_int(mulscale30(180, Sin(fix16_to_int(pPlayer->q16look)<<3)));
        else
            pPlayer->q16horiz = 0;
    }
    else
    {
        CONSTEXPR int upAngle = 289;
        CONSTEXPR int downAngle = -347;
        CONSTEXPR double lookStepUp = 4.0*upAngle/60.0;
        CONSTEXPR double lookStepDown = -4.0*downAngle/60.0;
        if (pInput->keyFlags.lookCenter && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
        {
            if (pPlayer->q16look < 0)
                pPlayer->q16look = fix16_min(pPlayer->q16look+F16(lookStepDown), F16(0));
            if (pPlayer->q16look > 0)
                pPlayer->q16look = fix16_max(pPlayer->q16look-F16(lookStepUp), F16(0));
            if (!pPlayer->q16look)
                pInput->keyFlags.lookCenter = 0;
        }
        else
        {
            if (pInput->buttonFlags.lookUp)
                pPlayer->q16look = fix16_min(pPlayer->q16look+F16(lookStepUp), F16(upAngle));
            if (pInput->buttonFlags.lookDown)
                pPlayer->q16look = fix16_max(pPlayer->q16look-F16(lookStepDown), F16(downAngle));
        }
        pPlayer->q16look = fix16_clamp(pPlayer->q16look+(pInput->q16mlook<<3), F16(downAngle), F16(upAngle));
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
        pPlayer->q16slopehoriz = interpolate(pPlayer->q16slopehoriz, F16(0), 0x4000);
        if (klabs(pPlayer->q16slopehoriz) < 4)
            pPlayer->q16slopehoriz = 0;
    }
    pPlayer->at83 = (-fix16_to_int(pPlayer->q16horiz))<<7;
    if (pInput->keyFlags.prevItem)
    {
        pInput->keyFlags.prevItem = 0;
        packPrevItem(pPlayer);
    }
    if (pInput->keyFlags.nextItem)
    {
        pInput->keyFlags.nextItem = 0;
        packNextItem(pPlayer);
    }
    if (pInput->keyFlags.useItem)
    {
        pInput->keyFlags.useItem = 0;
        if (pPlayer->packInfo[pPlayer->at321].at1 > 0)
            packUseItem(pPlayer, pPlayer->at321);
    }
    if (pInput->useFlags.useBeastVision)
    {
        pInput->useFlags.useBeastVision = 0;
        if (pPlayer->packInfo[3].at1 > 0)
            packUseItem(pPlayer, 3);
    }
    if (pInput->useFlags.useCrystalBall)
    {
        pInput->useFlags.useCrystalBall = 0;
        if (pPlayer->packInfo[2].at1 > 0)
            packUseItem(pPlayer, 2);
    }
    if (pInput->useFlags.useJumpBoots)
    {
        pInput->useFlags.useJumpBoots = 0;
        if (pPlayer->packInfo[4].at1 > 0)
            packUseItem(pPlayer, 4);
    }
    if (pInput->useFlags.useMedKit)
    {
        pInput->useFlags.useMedKit = 0;
        if (pPlayer->packInfo[0].at1 > 0)
            packUseItem(pPlayer, 0);
    }
    if (pInput->keyFlags.holsterWeapon)
    {
        pInput->keyFlags.holsterWeapon = 0;
        if (pPlayer->atbd)
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
    int nSprite = pPlayer->at5b;
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
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
    pPlayer->at6b = interpolate(pPlayer->at6b, zvel[nSprite], 0x7000);
    int dz = pPlayer->pSprite->z-pPosture->at24-pPlayer->at67;
    if (dz > 0)
        pPlayer->at6b += mulscale16(dz<<8, 0xa000);
    else
        pPlayer->at6b += mulscale16(dz<<8, 0x1800);
    pPlayer->at67 += pPlayer->at6b>>8;
    pPlayer->at73 = interpolate(pPlayer->at73, zvel[nSprite], 0x5000);
    dz = pPlayer->pSprite->z-pPosture->at28-pPlayer->at6f;
    if (dz > 0)
        pPlayer->at73 += mulscale16(dz<<8, 0x8000);
    else
        pPlayer->at73 += mulscale16(dz<<8, 0xc00);
    pPlayer->at6f += pPlayer->at73>>8;
    pPlayer->at37 = ClipLow(pPlayer->at37-4, 0);
    nSpeed >>= 16;
    if (pPlayer->at2f == 1)
    {
        pPlayer->at3b = (pPlayer->at3b+17)&2047;
        pPlayer->at4b = (pPlayer->at4b+17)&2047;
        pPlayer->at3f = mulscale30(pPosture->at14*10, Sin(pPlayer->at3b*2));
        pPlayer->at43 = mulscale30(pPosture->at18*pPlayer->at37, Sin(pPlayer->at3b-256));
        pPlayer->at4f = mulscale30(pPosture->at1c*pPlayer->at37, Sin(pPlayer->at4b*2));
        pPlayer->at53 = mulscale30(pPosture->at20*pPlayer->at37, Sin(pPlayer->at4b-0x155));
    }
    else
    {
        if (pXSprite->height < 256)
        {
            pPlayer->at3b = (pPlayer->at3b+pPosture->atc[pPlayer->at2e]*4) & 2047;
            pPlayer->at4b = (pPlayer->at4b+(pPosture->atc[pPlayer->at2e]*4)/2) & 2047;
            if (pPlayer->at2e)
            {
                if (pPlayer->at37 < 60)
                    pPlayer->at37 = ClipHigh(pPlayer->at37+nSpeed, 60);
            }
            else
            {
                if (pPlayer->at37 < 30)
                    pPlayer->at37 = ClipHigh(pPlayer->at37+nSpeed, 30);
            }
        }
        pPlayer->at3f = mulscale30(pPosture->at14*pPlayer->at37, Sin(pPlayer->at3b*2));
        pPlayer->at43 = mulscale30(pPosture->at18*pPlayer->at37, Sin(pPlayer->at3b-256));
        pPlayer->at4f = mulscale30(pPosture->at1c*pPlayer->at37, Sin(pPlayer->at4b*2));
        pPlayer->at53 = mulscale30(pPosture->at20*pPlayer->at37, Sin(pPlayer->at4b-0x155));
    }
    pPlayer->at35a = 0;
    pPlayer->at37f = ClipLow(pPlayer->at37f-4, 0);
    pPlayer->at35e = ClipLow(pPlayer->at35e-4, 0);
    pPlayer->at362 = ClipLow(pPlayer->at362-4, 0);
    pPlayer->at366 = ClipLow(pPlayer->at366-4, 0);
    pPlayer->at36a = ClipLow(pPlayer->at36a-4, 0);
    pPlayer->at377 = ClipLow(pPlayer->at377-4, 0);
    if (pPlayer == gMe && gMe->pXSprite->health == 0)
        pPlayer->at376 = 0;
    if (!pXSprite->health)
        return;
    pPlayer->at87 = 0;
    if (pPlayer->at2f == 1)
    {
        pPlayer->at87 = 1;
        int nSector = pSprite->sectnum;
        int nLink = gLowerLink[nSector];
        if (nLink > 0 && (sprite[nLink].type == 14 || sprite[nLink].type == 10))
        {
            if (getceilzofslope(nSector, pSprite->x, pSprite->y) > pPlayer->at67)
                pPlayer->at87 = 0;
        }
    }
    if (!pPlayer->at87)
    {
        pPlayer->at2f2 = 1200;
        pPlayer->at36e = 0;
        if (packItemActive(pPlayer, 1))
            packUseItem(pPlayer, 1);
    }
    int nType = kDudePlayer1-kDudeBase;
    switch (pPlayer->at2f)
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
    return actFireMissile(pPlayer->pSprite, a2, pPlayer->at6f-pPlayer->pSprite->z, a3, a4, a5, a6);
}

spritetype * playerFireThing(PLAYER *pPlayer, int a2, int a3, int thingType, int a5)
{
    dassert(thingType >= kThingBase && thingType < kThingMax);
    return actFireThing(pPlayer->pSprite, a2, pPlayer->at6f-pPlayer->pSprite->z, pPlayer->at83+a3, thingType, a5);
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
    if (myconnectindex == connecthead)
    {
        sprintf(buffer, "frag %d killed %d\n", pKiller->at57+1, pVictim->at57+1);
        sub_7AC28(buffer);
        buffer[0] = 0;
    }
    if (nKiller == nVictim)
    {
        pVictim->at2ee = -1;
        pVictim->at2c6--;
        pVictim->at2ca[nVictim]--;
        if (gGameOptions.nGameType == 3)
            dword_21EFB0[pVictim->at2ea]--;
        int nMessage = Random(5);
        int nSound = gSuicide[nMessage].at4;
        if (pVictim == gMe && gMe->at372 <= 0)
        {
            sprintf(buffer, "You killed yourself!");
            if (gGameOptions.nGameType > 0 && nSound >= 0)
                sndStartSample(nSound, 255, 2, 0);
        }
    }
    else
    {
        pKiller->at2c6++;
        pKiller->at2ca[nKiller]++;
        if (gGameOptions.nGameType == 3)
        {
            if (pKiller->at2ea == pVictim->at2ea)
                dword_21EFB0[pKiller->at2ea]--;
            else
            {
                dword_21EFB0[pKiller->at2ea]++;
                dword_21EFD0[pKiller->at2ea]+=120;
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
        PLAYER *pKiller = &gPlayer[pSprite->type-kDudePlayer1];
        playerFrag(pKiller, pPlayer);
        int nTeam1 = pKiller->at2ea&1;
        int nTeam2 = pPlayer->at2ea&1;
        if (nTeam1 == 0)
        {
            if (nTeam1 != nTeam2)
                evSend(0, 0, 15, COMMAND_ID_3);
            else
                evSend(0, 0, 16, COMMAND_ID_3);
        }
        else
        {
            if (nTeam1 == nTeam2)
                evSend(0, 0, 16, COMMAND_ID_3);
            else
                evSend(0, 0, 15, COMMAND_ID_3);
        }
    }
}

int playerDamageArmor(PLAYER *pPlayer, DAMAGE_TYPE nType, int nDamage)
{
    DAMAGEINFO *pDamageInfo = &damageInfo[nType];
    int nArmorType = pDamageInfo->at0;
    if (nArmorType >= 0 && pPlayer->at33e[nArmorType])
    {
#if 0
        int vbp = (nDamage*7)/8-nDamage/4;
        int v8 = pPlayer->at33e[nArmorType];
        int t = nDamage/4 + vbp * v8 / 3200;
        v8 -= t;
#endif
        int v8 = pPlayer->at33e[nArmorType];
        int t = scale(v8, 0, 3200, nDamage/4, (nDamage*7)/8);
        v8 -= t;
        nDamage -= t;
        pPlayer->at33e[nArmorType] = ClipLow(v8, 0);
    }
    return nDamage;
}

spritetype *sub_40A94(PLAYER *pPlayer, int a2)
{
    char buffer[80];
    spritetype *pSprite = NULL;
    switch (a2)
    {
    case 147:
        pPlayer->at90 &= ~1;
        pSprite = actDropObject(pPlayer->pSprite, 147);
        if (pSprite)
            pSprite->owner = pPlayer->at91[0];
        sprintf(buffer, "%s dropped Blue Flag", gProfile[pPlayer->at57].name);
        sndStartSample(8005, 255, 2, 0);
        viewSetMessage(buffer);
        break;
    case 148:
        pPlayer->at90 &= ~2;
        pSprite = actDropObject(pPlayer->pSprite, 148);
        if (pSprite)
            pSprite->owner = pPlayer->at91[1];
        sprintf(buffer, "%s dropped Red Flag", gProfile[pPlayer->at57].name);
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
    if (pPlayer->ata1[nDamageType])
        return 0;
    nDamage = playerDamageArmor(pPlayer, nDamageType, nDamage);
    pPlayer->at366 = ClipHigh(pPlayer->at366+(nDamage>>3), 600);

    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int nXSprite = pSprite->extra;
    int nXSector = sector[pSprite->sectnum].extra;
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nDeathSeqID = -1;
    int v18 = -1;
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
            if (nDamageType == DAMAGE_TYPE_4 && pXSprite->medium == 1 && !pPlayer->at376)
                nSound = 714;
            sfxPlay3DSound(pSprite, nSound, 0, 6);
            return nDamage;
        }
        sfxKill3DSound(pPlayer->pSprite, -1, 441);
        if (gGameOptions.nGameType == 3 && pPlayer->at90)
        {
            if (pPlayer->at90&1)
                sub_40A94(pPlayer, 147);
            if (pPlayer->at90&2)
                sub_40A94(pPlayer, 148);
        }
        pPlayer->at1fe = 0;
        pPlayer->at1b1 = 0;
        pPlayer->atbd = 0;
        pPlayer->at2ee = nSource;
        pPlayer->at34e = 0;
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
                v18 = nPlayerKeelClient;
                powerupActivate(pPlayer, 28);
                pXSprite->target = nSource;
                evPost(pSprite->index, 3, 15, CALLBACK_ID_13);
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
            trTriggerSector(pSprite->sectnum, &xsector[nXSector], 43);
        pSprite->hitag |= 7;
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (gPlayer[p].at2ee == nSprite && gPlayer[p].at1fe > 0)
                gPlayer[p].at2ee = -1;
        }
        FragPlayer(pPlayer, nSource);
        trTriggerSprite(nSprite, pXSprite, 0);
    }
    dassert(gSysRes.Lookup(pDudeInfo->seqStartID + nDeathSeqID, "SEQ") != NULL);
    seqSpawn(pDudeInfo->seqStartID+nDeathSeqID, 3, nXSprite, v18);
    return nDamage;
}

int UseAmmo(PLAYER *pPlayer, int nAmmoType, int nDec)
{
    if (gInfiniteAmmo)
        return 9999;
    if (nAmmoType == -1)
        return 9999;
    pPlayer->at181[nAmmoType] = ClipLow(pPlayer->at181[nAmmoType]-nDec, 0);
    return pPlayer->at181[nAmmoType];
}

void sub_41250(PLAYER *pPlayer)
{
    int v4 = pPlayer->at1be.dz;
    int dz = pPlayer->at6f-pPlayer->pSprite->z;
    if (UseAmmo(pPlayer, 9, 0) < 8)
    {
        pPlayer->at34e = 0;
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        int ang1 = (pPlayer->at352+pPlayer->at356)&2047;
        actFireVector(pPlayer->pSprite, 0, dz, Cos(ang1)>>16, Sin(ang1)>>16, v4, VECTOR_TYPE_21);
        int ang2 = (pPlayer->at352+2048-pPlayer->at356)&2047;
        actFireVector(pPlayer->pSprite, 0, dz, Cos(ang2)>>16, Sin(ang2)>>16, v4, VECTOR_TYPE_21);
    }
    pPlayer->at34e = ClipLow(pPlayer->at34e-1, 0);
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
                viewSetMessage("I LIVE...AGAIN!!");
            else
            {
                sprintf(buffer, "%s lives again!", gProfile[pPlayer->at57].name);
                viewSetMessage(buffer);
            }
            pPlayer->atc.newWeapon = 1;
        }
    }
}

void PlayerKeelsOver(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (gPlayer[p].pXSprite == pXSprite)
        {
            PLAYER *pPlayer = &gPlayer[p];
            playerDamageSprite(pPlayer->at2ee, pPlayer, DAMAGE_TYPE_5, 500<<4);
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
    for (int i = 0; i < gNetPlayers; i++)
    {
        gPlayer[i].pSprite = &sprite[gPlayer[i].at5b];
        gPlayer[i].pXSprite = &xsprite[gPlayer[i].pSprite->extra];
        gPlayer[i].pDudeInfo = &dudeInfo[gPlayer[i].pSprite->type-kDudeBase];
    }
}

void PlayerLoadSave::Save(void)
{
    Write(dword_21EFB0, sizeof(dword_21EFB0));
    Write(&gNetPlayers, sizeof(gNetPlayers));
    Write(&gProfile, sizeof(gProfile));
    Write(&gPlayer, sizeof(gPlayer));
}

static PlayerLoadSave *myLoadSave;

void PlayerLoadSaveConstruct(void)
{
    myLoadSave = new PlayerLoadSave();
}
