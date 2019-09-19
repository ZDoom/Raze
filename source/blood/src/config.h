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

#ifndef config_public_h_
#define config_public_h_

#include "keyboard.h"
#include "function.h"
#include "control.h"
#include "_control.h"
#include "hash.h"

#define MAXRIDECULE 10
#define MAXRIDECULELENGTH 40
#define MAXPLAYERNAME 16

extern int32_t MouseDeadZone, MouseBias;
extern int32_t SmoothInput;
extern int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
extern int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
extern int32_t MouseAnalogueAxes[MAXMOUSEAXES];
extern int32_t MouseAnalogueScale[MAXMOUSEAXES];
extern int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
extern int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
extern int32_t JoystickAnalogueAxes[MAXJOYAXES];
extern int32_t JoystickAnalogueScale[MAXJOYAXES];
extern int32_t JoystickAnalogueDead[MAXJOYAXES];
extern int32_t JoystickAnalogueSaturate[MAXJOYAXES];
extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
extern int32_t scripthandle;
extern int32_t setupread;
extern int32_t SoundToggle;
extern int32_t MusicToggle;
extern int32_t MusicRestartsOnLoadToggle;
extern int32_t CDAudioToggle;
extern int32_t FXVolume;
extern int32_t MusicVolume;
extern int32_t CDVolume;
extern int32_t NumVoices;
extern int32_t NumChannels;
extern int32_t NumBits;
extern int32_t MixRate;
extern int32_t ReverseStereo;
extern int32_t MusicDevice;
extern int32_t configversion;
extern int32_t CheckForUpdates;
extern int32_t LastUpdateCheck;
extern int32_t useprecache;
extern char CommbatMacro[MAXRIDECULE][MAXRIDECULELENGTH];
extern char szPlayerName[MAXPLAYERNAME];
extern int32_t gTurnSpeed;
extern int32_t gDetail;
extern int32_t gAutoAim;
extern int32_t gWeaponSwitch;
extern int32_t gAutoRun;
extern int32_t gViewInterpolate;
extern int32_t gViewHBobbing;
extern int32_t gViewVBobbing;
extern int32_t gFollowMap;
extern int32_t gOverlayMap;
extern int32_t gRotateMap;
extern int32_t gAimReticle;
extern int32_t gSlopeTilting;
extern int32_t gMessageState;
extern int32_t gMessageCount;
extern int32_t gMessageTime;
extern int32_t gMessageFont;
extern int32_t gbAdultContent;
extern char gzAdultPassword[9];
extern int32_t gDoppler;
extern int32_t gShowWeapon;
extern int32_t gMouseSensitivity;
extern int32_t gMouseAiming;
extern int32_t gMouseAimingFlipped;
extern int32_t gRunKeyMode;
extern bool gNoClip;
extern bool gInfiniteAmmo;
extern bool gFullMap;
extern hashtable_t h_gamefuncs;
extern int32_t gUpscaleFactor;
extern int32_t gBrightness;
extern int32_t gLevelStats;
extern int32_t gPowerupDuration;
extern int32_t gShowMapTitle;
extern int32_t MAXCACHE1DSIZE;
extern int32_t gFov;
extern int32_t gCenterHoriz;
extern int32_t gDeliriumBlur;

int  CONFIG_ReadSetup(void);
void CONFIG_WriteSetup(uint32_t flags);
void CONFIG_SetDefaults(void);
void CONFIG_SetupMouse(void);
void CONFIG_SetupJoystick(void);
void CONFIG_SetDefaultKeys(const char (*keyptr)[MAXGAMEFUNCLEN], bool lazy=false);

int32_t CONFIG_GetMapBestTime(char const *mapname, uint8_t const *mapmd4);
int     CONFIG_SetMapBestTime(uint8_t const *mapmd4, int32_t tm);

int32_t CONFIG_FunctionNameToNum(const char *func);
char *  CONFIG_FunctionNumToName(int32_t func);

int32_t     CONFIG_AnalogNameToNum(const char *func);
const char *CONFIG_AnalogNumToName(int32_t func);

void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2);

#endif
