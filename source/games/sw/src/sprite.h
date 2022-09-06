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

BEGIN_SW_NS

void KillActor(DSWActor* actor);
DSWActor* SpawnActor(int stat, int id, STATE* state, sectortype* sect, const DVector3& pos, DAngle ang, double vel = 0);

void SpriteSetup(void);
int move_actor(DSWActor* actor, const DVector3& change);
short GetSpriteDir(short sn);
short GetDirToPlayer(short sn);
short PlayerInVision(short sn, short view_deg);
short PlayerVisible(short sn);
bool SpriteOverlap(DSWActor*, DSWActor*);
int DoActorDie(DSWActor* actor, DSWActor* weapActor, int meansofdeath);
void SpriteControl(void);
void DoActorZrange(DSWActor*);
void PreMapCombineFloors(void);
void SpriteSetupPost(void);
int ActorCoughItem(DSWActor*);
bool ActorSpawn(DSWActor*);
int SpawnItemsMatch(short match);
void PicAnimOff(short picnum);
int MissileWaterAdjust(DSWActor*);
bool SpriteOverlapZ(DSWActor*, DSWActor*, double);

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
    int  amount;
};
extern struct InventoryDecl_t InventoryDecls[InvDecl_TOTAL];

END_SW_NS

#endif

