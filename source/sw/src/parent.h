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

#ifndef PARENT_PUBLIC_
#define PARENT_PUBLIC_

#define INVISTILE 6145

typedef struct TILE_INFO_TYPE
{
    short Voxel;    // Voxel Number to replace sprites with
    short Parental; // Tile offset to replace adult tiles with when locked out
    // 0 = Invisible
} ParentalStruct;

struct ORG_TILE;
typedef struct ORG_TILE OrgTile, *OrgTileP;
struct ORG_TILE_LIST;
typedef struct ORG_TILE_LIST OrgTileList, *OrgTileListP;

void JS_InitLockouts(void);
void JS_UnitInitLockouts(void);
void JS_ToggleLockouts(void);

struct ORG_TILE
{
    OrgTileP Next, Prev;
    short index;
    short orgpicnum;
};

struct ORG_TILE_LIST
{
    OrgTileP Next, Prev;
};

extern OrgTileList orgwalllist;                // The list containing orginal wall
// pics
extern OrgTileList orgwalloverlist;            // The list containing orginal wall
// over pics
extern OrgTileList orgsectorceilinglist;       // The list containing orginal sector
// ceiling pics
extern OrgTileList orgsectorfloorlist;         // The list containing orginal sector
// floor pics
#endif
