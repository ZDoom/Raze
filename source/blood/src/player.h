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
#pragma once
#include "compat.h"
#include "build.h"
#include "fix16.h"
#include "common_game.h"
#include "actor.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "db.h"
#include "dude.h"

enum LifeMode {
    kModeHuman = 0,
    kModeBeast,
    kModeHumanShrink,
    kModeHumanGrown,
};

struct PACKINFO {
    char at0; // is active (0/1)
    int at1 = 0; // remaining percent
};

struct PLAYER {
    spritetype *pSprite;
    XSPRITE *pXSprite;
    DUDEINFO *pDudeInfo;
    GINPUT atc;
    //short atc; // INPUT
    //char at10; // forward
    //short at11; // turn
    //char hearDist; // strafe
    //int at14; // buttonFlags
    //unsigned int at18; // keyFlags
    //char at1c; // useFlags;
    //char at20; // newWeapon
    //char at21; // mlook
    int at22;
    int at26; // weapon qav
    int at2a; // qav callback
    char at2e; // run
    int at2f; // state
    int at33; // unused?
    int at37;
    int at3b;
    int at3f; // bob height
    int at43; // bob width
    int at47;
    int at4b;
    int at4f; // bob sway y
    int at53; // bob sway x
    int at57; // Connect id
    int at5b; // spritenum
    int at5f; // life mode
    int at63;
    int at67; // view z
    int at6b;
    int at6f; // weapon z
    int at73;
    fix16_t q16look;
    int q16horiz; // horiz
    int q16slopehoriz; // horizoff
    int at83;
    char at87; // underwater
    char at88[8]; // keys
    char at90; // flag capture
    short at91[8];
    int ata1[7];
    char atbd; // weapon
    char atbe; // pending weapon
    int atbf, atc3, atc7;
    char atcb[14]; // hasweapon
    int atd9[14];
    int at111[2][14];
    //int at149[14];
    int at181[12]; // ammo
    char at1b1;
    int at1b2;
    int at1b6;
    int at1ba;
    Aim at1be; // world
    //int at1c6;
    Aim at1ca; // relative
    //int at1ca;
    //int at1ce;
    //int at1d2;
    int at1d6; // aim target sprite
    int at1da;
    short at1de[16];
    int at1fe;
    int at202[kMaxPowerUps]; // [13]: cloak of invisibility, [14]: death mask (invulnerability), [15]: jump boots, [17]: guns akimbo, [18]: diving suit, [21]: crystal ball, [24]: reflective shots, [25]: beast vision, [26]: cloak of shadow
    int at2c6; // frags
    int at2ca[8];
    int at2ea; // color (team)
    int at2ee; // killer
    int at2f2;
    int at2f6;
    int at2fa;
    int at2fe;
    int at302;
    int at306;
    int at30a;
    int at30e;
    int at312;
    int at316;
    char at31a; // God mode
    char at31b; // Fall scream
    char at31c;
    int at31d; // pack timer
    int at321; // pack id 1: diving suit, 2: crystal ball, 3: beast vision 4: jump boots
    PACKINFO packInfo[5]; // at325 [1]: diving suit, [2]: crystal ball, [3]: beast vision [4]: jump boots
    int at33e[3]; // armor
    //int at342;
    //int at346;
    int voodooTarget; // at34a
    int at34e;
    int at352;
    int at356;
    int at35a; // quake
    int at35e;
    int at362; // light
    int at366;
    int at36a; // blind
    int at36e; // choke
    int at372;
    char at376; // hand
    int at377;
    char at37b; // weapon flash
    int at37f; // quake2
    fix16_t q16ang;
    int angold;
    int player_par;
    int nWaterPal;
};

struct POSTURE {
    int at0;
    int at4;
    int at8;
    int atc[2];
    int at14;
    int at18;
    int at1c;
    int at20;
    int at24;
    int at28;
    int at2c;
    int at30;
};

struct PROFILE {
    int nAutoAim;
    int nWeaponSwitch;
    int skill;
    char name[MAXPLAYERNAME];
};

struct AMMOINFO {
    int at0;
    signed char at4;
};

struct POWERUPINFO
{
    short at0;
    char at2;
    int at3; // max value
    int at7;
};

extern POSTURE gPosture[4][3];

extern PLAYER gPlayer[kMaxPlayers];
extern PLAYER *gMe, *gView;

extern bool gBlueFlagDropped;
extern bool gRedFlagDropped;

extern PROFILE gProfile[kMaxPlayers];

extern int dword_21EFB0[kMaxPlayers];
extern ClockTicks dword_21EFD0[kMaxPlayers];
extern AMMOINFO gAmmoInfo[];
extern POWERUPINFO gPowerUpInfo[kMaxPowerUps];

int powerupCheck(PLAYER *pPlayer, int nPowerUp);
char powerupActivate(PLAYER *pPlayer, int nPowerUp);
void powerupDeactivate(PLAYER *pPlayer, int nPowerUp);
void powerupSetState(PLAYER *pPlayer, int nPowerUp, char bState);
void powerupProcess(PLAYER *pPlayer);
void powerupClear(PLAYER *pPlayer);
void powerupInit(void);
int packItemToPowerup(int nPack);
int powerupToPackItem(int nPowerUp);
char packAddItem(PLAYER *pPlayer, unsigned int nPack);
int packCheckItem(PLAYER *pPlayer, int nPack);
char packItemActive(PLAYER *pPlayer, int nPack);
void packUseItem(PLAYER *pPlayer, int nPack);
void packPrevItem(PLAYER *pPlayer);
void packNextItem(PLAYER *pPlayer);
char playerSeqPlaying(PLAYER * pPlayer, int nSeq);
void playerSetRace(PLAYER *pPlayer, int nLifeMode);
void playerSetGodMode(PLAYER *pPlayer, char bGodMode);
void playerResetInertia(PLAYER *pPlayer);
void playerCorrectInertia(PLAYER* pPlayer, vec3_t const *oldpos);
void playerStart(int nPlayer);
void playerReset(PLAYER *pPlayer);
void playerInit(int nPlayer, unsigned int a2);
char sub_3A158(PLAYER *a1, spritetype *a2);
char PickupItem(PLAYER *pPlayer, spritetype *pItem);
char PickupAmmo(PLAYER *pPlayer, spritetype *pAmmo);
char PickupWeapon(PLAYER *pPlayer, spritetype *pWeapon);
void PickUp(PLAYER *pPlayer, spritetype *pSprite);
void CheckPickUp(PLAYER *pPlayer);
int ActionScan(PLAYER *pPlayer, int *a2, int *a3);
void ProcessInput(PLAYER *pPlayer);
void playerProcess(PLAYER *pPlayer);
spritetype *playerFireMissile(PLAYER *pPlayer, int a2, int a3, int a4, int a5, int a6);
spritetype *playerFireThing(PLAYER *pPlayer, int a2, int a3, int thingType, int a5);
void playerFrag(PLAYER *pKiller, PLAYER *pVictim);
void FragPlayer(PLAYER *pPlayer, int nSprite);
int playerDamageArmor(PLAYER *pPlayer, DAMAGE_TYPE nType, int nDamage);
spritetype *sub_40A94(PLAYER *pPlayer, int a2);
int playerDamageSprite(int nSource, PLAYER *pPlayer, DAMAGE_TYPE nDamageType, int nDamage);
int UseAmmo(PLAYER *pPlayer, int nAmmoType, int nDec);
void sub_41250(PLAYER *pPlayer);
void playerLandingSound(PLAYER *pPlayer);
void PlayerSurvive(int, int nXSprite);
void PlayerKneelsOver(int, int nXSprite);
bool isGrown(spritetype* pSprite);
bool isShrinked(spritetype* pSprite);
bool shrinkPlayerSize(PLAYER* pPlayer, int divider);
bool growPlayerSize(PLAYER* pPlayer, int multiplier);
bool resetPlayerSize(PLAYER* pPlayer);
void deactivateSizeShrooms(PLAYER* pPlayer);
