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

#ifndef JSECTOR_H
#define JSECTOR_H

#include "reserve.h"

#define MAXMIRRORDIST       3300    // At this distance, or less, the magic mirrors activate.
#define MAXMIRRORMONSTERS   4       // Max monsters any one magic mirror can spawn

typedef enum
{
    m_normal, m_viewon, m_pissed
} MIRRORSTATE;

typedef struct
{
    short mirrorwall;                   // Wall number containing the mirror
    // tile
    short mirrorsector;                 // nextsector used internally to draw
    // mirror rooms
    short camera;                       // Contains number of ST1 sprite used
    // as a camera
    short camsprite;                    // sprite pointing to campic
    short campic;                       // Editart tile number to draw a
    // screen to
    short numspawnspots;                // Number of spawnspots used
    short spawnspots[MAXMIRRORMONSTERS]; // One spot for each possible skill
    // level for a
    // max of up to 4 coolie ghosts to spawn.
    SWBOOL ismagic;                       // Is this a magic mirror?
    MIRRORSTATE mstate;                 // What state the mirror is currently
    // in
    int maxtics;                       // Tic count used to time mirror
    // events
    int tics;                          // How much viewing time has been
    // used on mirror?
} MIRRORTYPE, *MIRRORTYPEp;

extern MIRRORTYPE mirror[MAXMIRRORS];

extern short mirrorcnt, floormirrorcnt;
extern short floormirrorsector[MAXMIRRORS];
extern SWBOOL mirrorinview;
extern short NormalVisibility;

void JAnalyzeSprites(uspritetype * tspr);
void JS_DrawMirrors(PLAYERp pp,int tx,int ty,int tz,short tpang,int tphoriz);
void JS_InitMirrors(void);
void JS_InitLockouts(void);
void JS_ToggleLockouts(void);
void JS_UnInitLockouts(void);
void JS_ProcessEchoSpot(void);
void JS_SpriteSetup(void);

#endif
