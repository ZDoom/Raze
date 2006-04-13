//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
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

extern int32 ControllerType;
extern int32 RunMode;
extern int32 AutoAim;
extern int32 ShowOpponentWeapons;
extern int32 ScreenMode;
extern int32 ScreenWidth;
extern int32 ScreenHeight;
extern int32 ScreenBPP;

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

void CONFIG_ReadSetup( void );
void CONFIG_GetSetupFilename( void );
void CONFIG_WriteSetup( void );
void CONFIG_SetupMouse( void );
void CONFIG_SetupJoystick( void );

#endif
