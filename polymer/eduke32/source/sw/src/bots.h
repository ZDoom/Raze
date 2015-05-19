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

// BOTS.H
// Contains useful header information for bot creation

#ifndef BOTS_H
#define BOTS_H


// BOT DEFINITIONS AND STRUCTURES

typedef enum
{
    BOTstand, BOThide, BOTrun, BOTduck, BOTjump, BOTstrafe, BOTshoot, BOTuseinv,
    BOTopen, BOTswimup, BOTswimdown, BOTturn, BOTuserts
} BOT_Actions;

// Linked lists containing node trees that are chosen based on desired actions
struct NODEstruct;
typedef struct NODEstruct NODE, *NODEp;

struct NODEstruct
{
    NODEp p, l, r;              // Pointers to tree nodes
    int goalx, goaly, goalz;   // x,y,z point bot wants to get to
    BOT_Actions action;         // Action to take if this node is reached
    int tics;                  // Optionally stay in this node for x tics.
};

struct NODETREEstruct;
typedef struct NODETREEstruct NODETREE, *NODETREEp;

struct NODETREEstruct
{
    short SpriteNum;        // Sprite number in sprite array of goal item
    NODEp tree;             // This is the node tree used to navigate to goal
    SWBOOL Locked;            // If list is locked, a bot is using/modifying it and
    // other bots cannot modify it while it's locked
};

// Bots main action variables
typedef struct BOT_BRAIN
{
    short tgt_inv;      // Inventory item it wants to use
    short tgt_weapon;   // Weapon in wants to activate and use
    short tgt_enemy;    // Enemy it wants to kill
    short tgt_sprite;   // Sprite it wants to pickup or operate
    short tgt_sector;   // Sector it wants to get to
    short tgt_wall;     // Wall it wants to touch
    BOT_Actions action; // Bot's current action
} BotBrain, *BotBrain_p;

// NOTE:
// The following arrays should be saved off with save games!

// 0  = Item not accessible, no item of type was found
// 1  = Shuriken
// 3  = Caltrops
// 4  = Gas Bomb
// 5  = Flash Bomb
// 6  = Uzi Ammo
// 7  = Shotgun Ammo
// 8  = Rocket Ammo
// 9  = 40mm Ammo
// 10 = Sticky Bombs
// 11 = Rail Ammo
// 12 = Head Ammo
// 13 = Heart Ammo
// 14 = Uzi
// 15 = Shotgun
// 16 = Rocket Launcher
// 17 = 40mm Launcher
// 18 = Rail Gun
// 19 = Head
// 20 = Heart
// 21 = MedKit
// 22 = Armor
// 23 = Big Armor
// 24 = Portable MedKit
// 25 = Fortune Cookie
////////////////////////
extern NODETREE BOT_TREELIST[25][50]; // There can be up to 50 of each item
// with a cooresponding search tree for each

#endif
