//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef _config_public
#define _config_public

#define SETUPNAMEPARM "SETUPFILE"

extern int32 FXDevice;
extern int32 MusicDevice;
extern int32 FXVolume;
extern int32 MusicVolume;
//extern fx_blaster_config BlasterConfig;
extern int32 NumVoices;
extern int32 NumChannels;
extern int32 NumBits;
extern int32 MixRate;
//extern int32 MidiPort;
extern int32 ReverseStereo;

extern int32 UseJoystick, UseMouse;
extern int32 RunMode;
extern int32 AutoAim;
extern int32 MouseFilter, MouseBias;
extern int32 SmoothInput;
extern int32 ShowOpponentWeapons;
extern int32 ScreenMode;
extern int32 ScreenWidth;
extern int32 ScreenHeight;
extern int32 ScreenBPP;
extern int32 ForceSetup;

extern byte KeyboardKeys[NUMGAMEFUNCTIONS][2];
extern int32 MouseFunctions[MAXMOUSEBUTTONS][2];
extern int32 MouseDigitalFunctions[MAXMOUSEAXES][2];
extern int32 MouseAnalogueAxes[MAXMOUSEAXES];
extern int32 MouseAnalogueScale[MAXMOUSEAXES];
extern int32 JoystickFunctions[MAXJOYBUTTONS][2];
extern int32 JoystickDigitalFunctions[MAXJOYAXES][2];
extern int32 JoystickAnalogueAxes[MAXJOYAXES];
extern int32 JoystickAnalogueScale[MAXJOYAXES];
extern int32 JoystickAnalogueDead[MAXJOYAXES];
extern int32 JoystickAnalogueSaturate[MAXJOYAXES];

#ifdef _WIN32
extern int32 checkforupdates, lastupdatecheck;
#endif

int32 CONFIG_ReadSetup( void );
void CONFIG_GetSetupFilename( void );
void CONFIG_WriteSetup( void );
void CONFIG_SetupMouse( void );
void CONFIG_SetupJoystick( void );
void CONFIG_SetDefaultKeys(void);

int32 CONFIG_GetMapBestTime(char *mapname);
int32 CONFIG_SetMapBestTime(char *mapname, int32 tm);

#endif
