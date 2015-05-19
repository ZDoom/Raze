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
#include "mytypes.h"
#include "gamedefs.h"
// Only ad to the end. These currently have to be in this order because of the
// way they are initilized.

typedef struct
{
    int MouseSpeed;
    int MusicVolume;
    int SoundVolume;
    CHAR BorderNum;
    CHAR Brightness;
    CHAR BorderTile;
    BOOL MouseAimingType;
    BOOL MouseLook;
    BOOL MouseInvert;
    BOOL Bobbing;
    BOOL Tilting;
    BOOL Shadows;
    BOOL AutoRun;
    BOOL Crosshair;
    BOOL AutoAim;
    BOOL Messages;
    BOOL FxOn;
    BOOL MusicOn;
    BOOL Talking;
    BOOL Ambient;
    BOOL FlipStereo;
// Net Options from Menus
    BYTE NetGameType;   // 0=DeathMatch [spawn], 1=Cooperative 2=DeathMatch [no spawn]
    BYTE NetLevel;      // 1-28
    BYTE NetMonsters;   // Cycle skill levels
    BOOL NetHurtTeammate;  // Allow friendly kills
    BOOL NetSpawnMarkers;    // Respawn markers on/off
    BOOL NetTeamPlay;   // Team play
    BYTE NetKillLimit;  // Number of frags at which game ends
    BYTE NetTimeLimit;  // Limit time of game
    BYTE NetColor;      // Chosen color for player
    BYTE ParentalLock;  // Parental Lock on/off
    char Password[20];  // Parental Lock password
    BOOL NetNuke;
    BOOL Voxels;
    BOOL Stats;
    BOOL MouseAimingOn; // whether it was on or off - NOT the type of mouse aiming
    BOOL PlayCD;
    char OggTrackName[MAXOGGTRACKLENGTH];
} GAME_SET, *GAME_SETp;

extern const GAME_SET gs_defaults;
extern GAME_SET gs;

