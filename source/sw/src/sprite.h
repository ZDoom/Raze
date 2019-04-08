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

#ifndef SPRITE_H

#define SPRITE_H

void KillSprite(int16_t SpriteNum);
int16_t SpawnSprite(short stat, short id, STATEp state, short sectnum, int x, int y, int z, int ang, int vel);
void SpriteSetup(void);
int move_actor(short SpriteNum, int xchange, int ychange, int zchange);
short GetSpriteDir(short sn);
short GetDirToPlayer(short sn);
short PlayerInVision(short sn, short view_deg);
short PlayerVisible(short sn);
short SpriteLookSector(short SpriteNum, short range);
short SpriteCanGoForward(short SpriteNum, short range);
void  SpriteFindNewDirection(short SpriteNum, short range);
int DoWalk(short SpriteNum);
int DoBody(short SpriteNum);
SWBOOL CanMoveHere(int16_t spritenum);
SWBOOL SpriteOverlap(int16_t spritenum_a, int16_t spritenum_b);
int DoActorDie(short SpriteNum, short weapon);
int DoGet(short SpriteNum);
void SpriteControl(void);
void SetEnemyInactive(short SpriteNum);
void DoActorZrange(short SpriteNum);
void PreMapCombineFloors(void);
void SpriteSetupPost(void);
int ActorCoughItem(short SpriteNum);
SWBOOL ActorSpawn(SPRITEp sp);
int SpawnItemsMatch(short match);
void PicAnimOff(short picnum);
int MissileWaterAdjust(short SpriteNum);
SWBOOL SpriteOverlapZ(int16_t spritenum_a,int16_t spritenum_b,int z_overlap);

enum
{
    InvDecl_Armor,      // ie. +50 armour
    InvDecl_Kevlar,     // ie. +100 armour
    InvDecl_SmMedkit,
    InvDecl_Booster,    // ie. fortune cookie
    InvDecl_Medkit,
    InvDecl_ChemBomb,   // ie. gas bomb
    InvDecl_FlashBomb,
    InvDecl_Caltrops,
    InvDecl_NightVision,
    InvDecl_RepairKit,
    InvDecl_Cloak,
    InvDecl_TOTAL
};

struct InventoryDecl_t
{
    const char *name;
    int  amount;
};
extern struct InventoryDecl_t InventoryDecls[InvDecl_TOTAL];

#endif

