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
    int8_t BorderNum;
    int8_t Brightness;
    int8_t BorderTile;
    SWBOOL MouseAimingType;
    SWBOOL MouseLook;
    SWBOOL MouseInvert;
    SWBOOL Bobbing;
    SWBOOL Tilting;
    SWBOOL Shadows;
    SWBOOL AutoRun;
    SWBOOL Crosshair;
    SWBOOL AutoAim;
    SWBOOL Messages;
    SWBOOL FxOn;
    SWBOOL MusicOn;
    SWBOOL Talking;
    SWBOOL Ambient;
    SWBOOL FlipStereo;
// Net Options from Menus
    uint8_t NetGameType;   // 0=DeathMatch [spawn], 1=Cooperative 2=DeathMatch [no spawn]
    uint8_t NetLevel;      // 1-28
    uint8_t NetMonsters;   // Cycle skill levels
    SWBOOL NetHurtTeammate;  // Allow friendly kills
    SWBOOL NetSpawnMarkers;    // Respawn markers on/off
    SWBOOL NetTeamPlay;   // Team play
    uint8_t NetKillLimit;  // Number of frags at which game ends
    uint8_t NetTimeLimit;  // Limit time of game
    uint8_t NetColor;      // Chosen color for player
    uint8_t ParentalLock;  // Parental Lock on/off
    char Password[20];  // Parental Lock password
    SWBOOL NetNuke;
    SWBOOL Voxels;
    SWBOOL Stats;
    SWBOOL MouseAimingOn; // whether it was on or off - NOT the type of mouse aiming
    SWBOOL PlayCD;
    char WaveformTrackName[MAXWAVEFORMTRACKLENGTH];
} GAME_SET, *GAME_SETp;

extern const GAME_SET gs_defaults;
extern GAME_SET gs;

