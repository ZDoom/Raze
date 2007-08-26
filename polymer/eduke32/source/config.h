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

typedef struct {
	//
	// Sound variables
	//
	int32 FXDevice;
	int32 MusicDevice;
	int32 FXVolume;
	int32 MusicVolume;
	int32 SoundToggle;
	int32 MusicToggle;
	int32 VoiceToggle;
	int32 AmbienceToggle;

	int32 NumVoices;
	int32 NumChannels;
	int32 NumBits;
	int32 MixRate;
	
	int32 ReverseStereo;

	int32 UseJoystick;
	int32 UseMouse;
	int32 RunMode;
	int32 AutoAim;
	int32 ShowOpponentWeapons;
	int32 MouseFilter,MouseBias;
	int32 SmoothInput;

	// JBF 20031211: Store the input settings because
	// (currently) jmact can't regurgitate them
	byte KeyboardKeys[NUMGAMEFUNCTIONS][2];
	int32 MouseFunctions[MAXMOUSEBUTTONS][2];
	int32 MouseDigitalFunctions[MAXMOUSEAXES][2];
	int32 MouseAnalogueAxes[MAXMOUSEAXES];
	int32 MouseAnalogueScale[MAXMOUSEAXES];
	int32 JoystickFunctions[MAXJOYBUTTONS][2];
	int32 JoystickDigitalFunctions[MAXJOYAXES][2];
	int32 JoystickAnalogueAxes[MAXJOYAXES];
	int32 JoystickAnalogueScale[MAXJOYAXES];
	int32 JoystickAnalogueDead[MAXJOYAXES];
	int32 JoystickAnalogueSaturate[MAXJOYAXES];

	//
	// Screen variables
	//

	int32 ScreenMode;

	int32 ScreenWidth;
	int32 ScreenHeight;
	int32 ScreenBPP;

	int32 ForceSetup;

	int32 scripthandle;
	int32 setupread;

	int32 CheckForUpdates;
	int32 LastUpdateCheck;
	int32 useprecache;
} config_t;

extern config_t config;

int32 CONFIG_ReadSetup( void );
void CONFIG_GetSetupFilename( void );
void CONFIG_WriteSetup( void );
void CONFIG_SetupMouse( void );
void CONFIG_SetupJoystick( void );
void CONFIG_SetDefaultKeys(int type);

int32 CONFIG_GetMapBestTime(char *mapname);
int32 CONFIG_SetMapBestTime(char *mapname, int32 tm);

#endif
