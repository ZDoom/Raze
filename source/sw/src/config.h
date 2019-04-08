//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#ifndef config_public_
#define config_public_

#include "compat.h"

#include "function.h"
#include "keyboard.h"
#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SETUPNAMEPARM "SETUPFILE"

// screen externs
extern int32_t ScreenMode; // Screen mode
extern int32_t ScreenWidth;
extern int32_t ScreenHeight;
extern int32_t ScreenBPP;
extern int32_t ScreenBufferMode;
extern int32_t VesaBufferMode;
extern int32_t ForceSetup;

// sound externs
extern int32_t FXDevice; // Sound FX Card number
extern int32_t MusicDevice; // Music Card number
extern int32_t FXVolume; // FX Volume
extern int32_t MusicVolume; // Music Volume
extern int32_t NumVoices; // Number of voices
extern int32_t NumChannels; // Number of channels
extern int32_t NumBits; // Number of bits
extern int32_t MixRate; // Mixing rate
extern int32_t MidiPort; // Midi Port
extern int32_t ReverseStereo; // Reverse Stereo Channels

// comm externs
extern int32_t ComPort;
extern int32_t IrqNumber;
extern int32_t UartAddress;
extern int32_t PortSpeed;

extern int32_t ToneDial;
extern char  ModemName[MAXMODEMSTRING];
extern char  InitString[MAXMODEMSTRING];
extern char  HangupString[MAXMODEMSTRING];
extern char  DialoutString[MAXMODEMSTRING];
extern int32_t SocketNumber;
extern char  CommbatMacro[MAXMACROS][MAXMACROLENGTH];
extern char  PhoneNames[MAXPHONEENTRIES][PHONENAMELENGTH];
extern char  PhoneNumbers[MAXPHONEENTRIES][PHONENUMBERLENGTH];
extern char  PhoneNumber[PHONENUMBERLENGTH];
extern int32_t NumberPlayers;
extern int32_t ConnectType;
extern char  PlayerName[MAXPLAYERNAMELENGTH];
extern char  RTSName[MAXRTSNAMELENGTH];
extern char  UserLevel[MAXUSERLEVELNAMELENGTH];
extern char  RTSPath[MAXRTSPATHLENGTH];
extern char  UserPath[MAXUSERLEVELPATHLENGTH];

// controller externs
extern int32_t UseMouse, UseJoystick;
extern int32_t JoystickPort;
extern int32_t MouseSensitivity;
extern int32_t MouseAiming;
extern int32_t MouseAimingFlipped;

extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];

extern int32_t MouseButtons[MAXMOUSEBUTTONS];
extern int32_t MouseButtonsClicked[MAXMOUSEBUTTONS];

extern int32_t JoystickButtons[MAXJOYBUTTONS];
extern int32_t JoystickButtonsClicked[MAXJOYBUTTONS];

extern int32_t MouseAnalogAxes[MAXMOUSEAXES];
extern int32_t JoystickAnalogAxes[MAXJOYAXES];
extern int32_t MouseAnalogScale[MAXMOUSEAXES];
extern int32_t JoystickAnalogScale[MAXJOYAXES];
extern int32_t JoystickAnalogDead[MAXJOYAXES];
extern int32_t JoystickAnalogSaturate[MAXJOYAXES];

extern int32_t EnableRudder;

extern int32_t MouseDigitalAxes[MAXMOUSEAXES][2];
extern int32_t JoystickDigitalAxes[MAXJOYAXES][2];

extern char setupfilename[BMAX_PATH];
extern char ExternalControlFilename[64];

//style=0: classic
//style=1: modern
void SetMouseDefaults(int style);
void SetJoystickDefaults(void);
void SetDefaultKeyDefinitions(int style);

int32_t CONFIG_ReadSetup(void);
void CONFIG_SetupMouse(void);
void CONFIG_SetupJoystick(void);
void CONFIG_WriteSetup(void);
void WriteCommitFile(int32_t gametype);
void CONFIG_GetSetupFilename(void);

const char *CONFIG_FunctionNumToName(int32_t func);
int32_t CONFIG_FunctionNameToNum(const char *func);
const char *CONFIG_AnalogNumToName(int32_t func);
int32_t CONFIG_AnalogNameToNum(const char *func);

#ifdef __cplusplus
};
#endif
#endif
