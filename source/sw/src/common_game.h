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

//****************************************************************************
//
// common_game.h
//
// common defines for the setup program
//
//****************************************************************************

#ifndef common_public_
#define common_public_
#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
//
// DEFINES
//
//****************************************************************************

#ifndef APPBASENAME
#define APPBASENAME         "voidsw"
#endif

//
// Color Defines
//

#define MENUBACK_FOREGROUND COLOR_BLACK
#define MENUBACK_BACKGROUND COLOR_DARKGRAY

#define MENUBACKBORDER_FOREGROUND COLOR_BLACK
#define MENUBACKBORDER_BACKGROUND COLOR_GRAY

#define MENU_ACTIVE_FOREGROUND   COLOR_WHITE
#define MENU_INACTIVE_FOREGROUND COLOR_GRAY
#define MENU_DISPLAY_FOREGROUND  COLOR_LIGHTGREEN

#define MENU_SECTIONHEADER_FOREGROUND   COLOR_YELLOW

//
// Setup program defines
//

#define SETUPPROGRAMNAME ("Shadow Warrior Setup")
#define SETUPPROGRAMVERSION ("1.2")

#define GAMENAME "Shadow Warrior"
#define GAMELAUNCHER ("SW.EXE")
#define GAMETOTYPE ("SW")

#define MENUFOOTER "Esc Exits   Move  ды Selects\0"

#define COMMITLAUNCHER ("COMMIT.EXE")
#define COMMITFILENAME ("COMMIT.DAT")

#define MAXVOICES 32
#define SONGNAME ("Shadow Warrior Theme Song")
//#define SOUNDSETUPLAUNCHER ("SNDSETUP.EXE")

// Default Socket Number

#define DEFAULTSOCKETNUMBER 0x8849

// Default RTS file

#define DEFAULTRTSFILE "sw.rts"

// Default RTS path

#define DEFAULTRTSPATH ".\\"

// Default UserLevel path

#define DEFAULTLEVELPATH ".\\"

// Default External Control file

#define DEFAULTCONTROLFILE "EXTERNAL.EXE"

// Default Help file

#define DEFAULTHELPFILE "SWHELP.EXE"

// RTS extension

#define RTSEXTENSION "RTS"

// MAP extension

#define MAPEXTENSION "MAP"

// Default Player name

#define DEFAULTPLAYERNAME "KATO"

// Default Macros

#define MACRO1  "Burn baby burn..."
#define MACRO2  "You make another stupid move."
#define MACRO3  "Blocking with your head again?"
#define MACRO4  "You not fight well with hands!"
#define MACRO5  "You so stupid!"
#define MACRO6  "Quit jerking off. Come fight me!"
#define MACRO7  "What the matter you scaredy cat?"
#define MACRO8  "Did I break your concentration?"
#define MACRO9  "Hope you were paying attention."
#define MACRO10 "ITTAIIIUUU!!!"


extern char *g_grpNamePtr;

const char *G_DefaultGrpFile(void);
const char *G_GrpFile(void);

void clearGrpNamePtr(void);

void SW_InitMultiPsky(void);

#ifdef __cplusplus
};
#endif
#endif
