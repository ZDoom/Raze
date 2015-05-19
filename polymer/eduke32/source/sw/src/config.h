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
#ifdef __cplusplus
extern "C" {
#endif

#include "function.h"

#define SETUPNAMEPARM "SETUPFILE"

// screen externs
extern int32 ScreenMode; // Screen mode
extern int32 ScreenWidth;
extern int32 ScreenHeight;
extern int32 ScreenBPP;
extern int32 ScreenBufferMode;
extern int32 VesaBufferMode;
extern int32 ForceSetup;

// sound externs
extern int32 FXDevice; // Sound FX Card number
extern int32 MusicDevice; // Music Card number
extern int32 FXVolume; // FX Volume
extern int32 MusicVolume; // Music Volume
extern int32 NumVoices; // Number of voices
extern int32 NumChannels; // Number of channels
extern int32 NumBits; // Number of bits
extern int32 MixRate; // Mixing rate
extern int32 MidiPort; // Midi Port
extern int32 ReverseStereo; // Reverse Stereo Channels

// comm externs
extern int32 ComPort;
extern int32 IrqNumber;
extern int32 UartAddress;
extern int32 PortSpeed;

extern int32 ToneDial;
extern char  ModemName[MAXMODEMSTRING];
extern char  InitString[MAXMODEMSTRING];
extern char  HangupString[MAXMODEMSTRING];
extern char  DialoutString[MAXMODEMSTRING];
extern int32 SocketNumber;
extern char  CommbatMacro[MAXMACROS][MAXMACROLENGTH];
extern char  PhoneNames[MAXPHONEENTRIES][PHONENAMELENGTH];
extern char  PhoneNumbers[MAXPHONEENTRIES][PHONENUMBERLENGTH];
extern char  PhoneNumber[PHONENUMBERLENGTH];
extern int32 NumberPlayers;
extern int32 ConnectType;
extern char  PlayerName[MAXPLAYERNAMELENGTH];
extern char  RTSName[MAXRTSNAMELENGTH];
extern char  UserLevel[MAXUSERLEVELNAMELENGTH];
extern char  RTSPath[MAXRTSPATHLENGTH];
extern char  UserPath[MAXUSERLEVELPATHLENGTH];

// controller externs
extern int32 UseMouse, UseJoystick;
extern int32 JoystickPort;
extern int32 MouseSensitivity;
extern int32 MouseAiming;
extern int32 MouseAimingFlipped;

extern byte KeyboardKeys[NUMGAMEFUNCTIONS][2];

extern int32 MouseButtons[MAXMOUSEBUTTONS];
extern int32 MouseButtonsClicked[MAXMOUSEBUTTONS];

extern int32 JoystickButtons[MAXJOYBUTTONS];
extern int32 JoystickButtonsClicked[MAXJOYBUTTONS];

extern int32 MouseAnalogAxes[MAXMOUSEAXES];
extern int32 JoystickAnalogAxes[MAXJOYAXES];
extern int32 MouseAnalogScale[MAXMOUSEAXES];
extern int32 JoystickAnalogScale[MAXJOYAXES];
extern int32 JoystickAnalogDead[MAXJOYAXES];
extern int32 JoystickAnalogSaturate[MAXJOYAXES];

extern int32 EnableRudder;

extern int32 MouseDigitalAxes[MAXMOUSEAXES][2];
extern int32 JoystickDigitalAxes[MAXJOYAXES][2];

extern char setupfilename[64];
extern char ExternalControlFilename[64];

//style=0: classic
//style=1: modern
void SetMouseDefaults(int style);
void SetJoystickDefaults(void);
void SetDefaultKeyDefinitions(int style);

int32 CONFIG_ReadSetup(void);
void CONFIG_SetupMouse(void);
void CONFIG_SetupJoystick(void);
void CONFIG_WriteSetup(void);
void WriteCommitFile(int32 gametype);
void CONFIG_GetSetupFilename(void);

const char *CONFIG_FunctionNumToName(int32 func);
int32 CONFIG_FunctionNameToNum(const char *func);
const char *CONFIG_AnalogNumToName(int32 func);
int32 CONFIG_AnalogNameToNum(const char *func);

#ifdef __cplusplus
};
#endif
#endif
